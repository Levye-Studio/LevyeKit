#include "ConversionBackup.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <set>
#include <sstream>
#include <vector>

#include "SHA256.hpp"

namespace Levye {
namespace {
namespace fs = std::filesystem;

struct Record {
  fs::path relative;
  bool existed = false;
  std::string before;
  std::string after;
};

bool ReadRegular(const fs::path& path, std::string& result) {
  std::error_code ec;
  if (!fs::is_regular_file(fs::symlink_status(path, ec)) || ec)
    return false;
  std::ifstream in(path, std::ios::binary);
  if (!in)
    return false;
  result.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  return !in.bad();
}

bool WriteRegular(const fs::path& path, const std::string& bytes) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out)
    return false;
  out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  out.close();
  return out.good();
}

bool SafeRelative(const fs::path& path) {
  if (path.empty() || path.is_absolute())
    return false;
  for (const auto& part : path)
    if (part == "." || part == ".." || part.empty())
      return false;
  return true;
}

bool NoSymlinkComponents(const fs::path& root, const fs::path& relative) {
  fs::path current = root;
  for (const auto& part : relative) {
    current /= part;
    std::error_code ec;
    if (fs::is_symlink(fs::symlink_status(current, ec)) ||
        (ec && ec != std::errc::no_such_file_or_directory))
      return false;
  }
  return true;
}

bool ValidDigest(const std::string& digest) {
  if (digest.size() != 64)
    return false;
  for (char c : digest)
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
      return false;
  return true;
}

bool ReadManifest(const fs::path& backup, std::vector<Record>& records) {
  std::string manifest, expected;
  if (!ReadRegular(backup / "manifest.txt", manifest) ||
      !ReadRegular(backup / "manifest.sha256", expected) || expected != SHA256(manifest) + "\n")
    return false;
  std::istringstream in(manifest);
  std::string header;
  if (!(in >> header) || header != "levye-convert-backup-v2")
    return false;
  std::set<fs::path> seen;
  while (in >> std::ws && !in.eof()) {
    Record record;
    std::string relative;
    if (!(in >> std::quoted(relative) >> record.existed >> record.before >> record.after))
      return false;
    record.relative = relative;
    if (!SafeRelative(record.relative) || !seen.insert(record.relative).second ||
        !ValidDigest(record.after) ||
        (record.existed ? !ValidDigest(record.before) : record.before != "-"))
      return false;
    records.push_back(std::move(record));
  }
  return !in.bad() && !records.empty();
}

bool ReadState(const fs::path& backup, bool& started, bool& complete) {
  std::error_code ec;
  const auto startPath = backup / "rollback.started";
  const auto completePath = backup / "rollback.complete";
  started = fs::exists(startPath, ec) && !ec;
  ec.clear();
  complete = fs::exists(completePath, ec) && !ec;
  if (ec || (complete && !started))
    return false;
  std::string marker;
  if (started && (!ReadRegular(startPath, marker) || marker != "levye-convert-rollback-v1\n"))
    return false;
  if (complete && (!ReadRegular(completePath, marker) || marker != "levye-convert-rollback-v1\n"))
    return false;
  return true;
}

bool Marker(const fs::path& path) {
  std::error_code ec;
  if (fs::exists(path, ec) || ec)
    return false;
  return WriteRegular(path, "levye-convert-rollback-v1\n");
}

}  // namespace

bool SaveConversionBackup(const fs::path& root, const std::vector<PendingFile>& files,
                          fs::path& backup, std::string& error) {
  backup = root / ".levye-convert-backup";
  for (int suffix = 2; fs::exists(backup); ++suffix)
    backup = root / (".levye-convert-backup-" + std::to_string(suffix));
  std::error_code ec;
  if (!fs::create_directory(backup, ec) || ec) {
    error = "Cannot create conversion backup: " + backup.string();
    backup.clear();
    return false;
  }
  std::ostringstream manifest;
  manifest << "levye-convert-backup-v2\n";
  for (const auto& file : files) {
    const fs::path relative = file.path.lexically_relative(root);
    if (!SafeRelative(relative)) {
      error = "Unsafe conversion output: " + file.path.string();
      return false;
    }
    std::string original;
    const bool existed = fs::exists(file.path);
    if (existed) {
      if (!ReadRegular(file.path, original)) {
        error = "Cannot read original conversion file: " + file.path.string();
        return false;
      }
      const auto saved = backup / "original" / relative;
      fs::create_directories(saved.parent_path(), ec);
      if (ec || !fs::copy_file(file.path, saved, fs::copy_options::none, ec) || ec) {
        error = "Cannot back up conversion file: " + file.path.string();
        return false;
      }
    }
    manifest << std::quoted(relative.generic_string()) << ' ' << existed << ' '
             << (existed ? SHA256(original) : "-") << ' ' << SHA256(file.content) << '\n';
  }
  const auto text = manifest.str();
  if (!WriteRegular(backup / "manifest.txt", text) ||
      !WriteRegular(backup / "manifest.sha256", SHA256(text) + "\n")) {
    error = "Cannot write conversion backup manifest.";
    return false;
  }
  return true;
}

bool RollbackConversion(const fs::path& root, std::string& error) {
  std::error_code ec;
  if (fs::is_symlink(fs::symlink_status(root, ec)) || ec) {
    error = "Project root is a symlink or cannot be inspected.";
    return false;
  }
  const fs::path projectRoot = fs::canonical(root, ec);
  if (ec || !fs::is_directory(projectRoot)) {
    error = "Project root is missing or unsafe.";
    return false;
  }
  fs::path backup = projectRoot / ".levye-convert-backup";
  for (int suffix = 2;
       fs::exists(projectRoot / (".levye-convert-backup-" + std::to_string(suffix))); ++suffix)
    backup = projectRoot / (".levye-convert-backup-" + std::to_string(suffix));
  if (!fs::is_directory(fs::symlink_status(backup, ec)) || ec ||
      !NoSymlinkComponents(backup, "manifest.txt") ||
      !NoSymlinkComponents(backup, "manifest.sha256")) {
    error = "Conversion backup directory is unsafe.";
    return false;
  }
  std::vector<Record> records;
  if (!ReadManifest(backup, records)) {
    error = "Conversion backup metadata is missing or corrupt: " + backup.string();
    return false;
  }
  bool started = false, complete = false;
  if (!ReadState(backup, started, complete)) {
    error = "Conversion rollback state is invalid.";
    return false;
  }
  if (complete)
    return true;

  std::vector<PendingFile> restores;
  std::vector<fs::path> removals;
  for (const auto& record : records) {
    const auto output = projectRoot / record.relative;
    if (!NoSymlinkComponents(projectRoot, record.relative)) {
      error = "Unsafe converted path: " + output.string();
      return false;
    }
    std::string current;
    const bool present = ReadRegular(output, current);
    if (!present && fs::exists(output)) {
      error = "Cannot read converted file: " + output.string();
      return false;
    }
    if (record.existed) {
      std::string original;
      if (!NoSymlinkComponents(backup, fs::path("original") / record.relative) ||
          !ReadRegular(backup / "original" / record.relative, original) ||
          SHA256(original) != record.before) {
        error = "Conversion backup changed: " + record.relative.string();
        return false;
      }
      if (present && SHA256(current) == record.before && started)
        continue;
      if ((present && SHA256(current) == record.after) || (!present && started))
        restores.push_back({output, std::move(original), true});
      else {
        error = "Converted file changed or is missing; refusing rollback: " + output.string();
        return false;
      }
    } else {
      if (present && SHA256(current) == record.after)
        removals.push_back(output);
      else if (present || !started) {
        error = "Generated file changed or is missing; refusing rollback: " + output.string();
        return false;
      }
    }
  }
  if (!started && !Marker(backup / "rollback.started")) {
    error = "Cannot record rollback start; no files changed.";
    return false;
  }
  // The marker makes a later invocation recoverable after a process interruption.
  if (!restores.empty() && !CommitFiles(projectRoot, restores, error))
    return false;
  for (const auto& output : removals) {
    std::string current;
    const auto relative = output.lexically_relative(projectRoot);
    if (!NoSymlinkComponents(projectRoot, relative)) {
      error = "Generated path changed during rollback: " + output.string();
      return false;
    }
    if (!ReadRegular(output, current))
      continue;
    const auto it = std::find_if(records.begin(), records.end(),
                                 [&](const Record& record) { return record.relative == relative; });
    if (it == records.end() || SHA256(current) != it->after) {
      error = "Generated file changed during rollback: " + output.string();
      return false;
    }
    ec.clear();
    if (!fs::remove(output, ec) || ec) {
      error = "Cannot remove generated file during rollback: " + output.string();
      return false;
    }
  }
  if (!Marker(backup / "rollback.complete")) {
    error = "Files restored, but could not record rollback completion.";
    return false;
  }
  return true;
}
}  // namespace Levye
