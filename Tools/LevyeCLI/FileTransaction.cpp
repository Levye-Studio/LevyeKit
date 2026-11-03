
#include "FileTransaction.hpp"

#include <fstream>
#include <random>
#include <string>
#include <system_error>
#include <unordered_set>
#include <vector>

namespace Levye {
namespace {

namespace fs = std::filesystem;

struct Entry {
  fs::path destination;
  fs::path staged;
  fs::path backup;

  bool hadOriginal = false;
  bool movedOriginal = false;
  bool installed = false;
};

bool Inspect(const fs::path& path, bool& exists) {
  std::error_code ec;
  const auto status = fs::symlink_status(path, ec);

  if (ec == std::errc::no_such_file_or_directory) {
    exists = false;
    return true;
  }

  if (ec) return false;

  exists = fs::exists(status);
  return !exists || fs::is_regular_file(status);
}

bool Write(const fs::path& path, const std::string& content) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) return false;

  out.write(content.data(), static_cast<std::streamsize>(content.size()));
  out.close();

  return out.good();
}

bool ShouldFail(const TransactionFailureInjection* injection, TransactionFailurePoint point,
                std::size_t index) {
  return injection && injection->point == point && injection->fileIndex == index;
}

}  // namespace

bool CommitFiles(const fs::path& projectRoot, const std::vector<PendingFile>& files,
                 std::string& error, const TransactionFailureInjection* injection) {
  error.clear();
  if (files.empty()) return true;

  std::error_code ec;
  std::mt19937_64 rng(std::random_device{}());

  fs::path work;
  bool created = false;

  for (int i = 0; i < 32; ++i) {
    const auto candidate = projectRoot / (".levye-transaction-" + std::to_string(rng()));

    ec.clear();

    if (fs::create_directory(candidate, ec)) {
      work = candidate;
      created = true;
      break;
    }

    if (ec != std::errc::file_exists) {
      error = "Cannot create transaction directory: " + ec.message();
      return false;
    }
  }

  if (!created) {
    error = "Cannot allocate transaction directory.";
    return false;
  }

  std::vector<Entry> entries;
  entries.reserve(files.size());

  std::unordered_set<std::string> destinations;
  bool success = true;

  // Phase 1: Validate and stage every output.
  for (std::size_t i = 0; i < files.size(); ++i) {
    const auto& file = files[i];

    Entry entry;
    entry.destination = file.path;
    entry.staged = work / (std::to_string(i) + ".stage");
    entry.backup = work / (std::to_string(i) + ".backup");

    const auto normalized = entry.destination.lexically_normal().string();

    if (!destinations.insert(normalized).second) {
      error = "Duplicate transaction destination: " + normalized;
      success = false;
      break;
    }

    if (!Inspect(entry.destination, entry.hadOriginal) ||
        (entry.hadOriginal && !file.allowReplace)) {
      error = "Unsafe or existing destination: " + entry.destination.string();

      success = false;
      break;
    }

    if (!Write(entry.staged, file.content)) {
      error = "Cannot stage: " + entry.destination.string();
      success = false;
      break;
    }

    entries.push_back(std::move(entry));
  }

  // Phase 2: Commit staged outputs.
  if (success) {
    for (std::size_t i = 0; i < entries.size(); ++i) {
      auto& entry = entries[i];

      if (ShouldFail(injection, TransactionFailurePoint::BeforeBackup, i)) {
        error = "Injected failure before backup.";
        success = false;
        break;
      }

      if (entry.hadOriginal) {
        ec.clear();
        fs::rename(entry.destination, entry.backup, ec);

        if (ec) {
          error = "Cannot back up " + entry.destination.string() + ": " + ec.message();

          success = false;
          break;
        }

        entry.movedOriginal = true;
      }

      if (ShouldFail(injection, TransactionFailurePoint::AfterBackup, i)) {
        error = "Injected failure after backup.";
        success = false;
        break;
      }

      if (ShouldFail(injection, TransactionFailurePoint::BeforeInstall, i)) {
        error = "Injected failure before install.";
        success = false;
        break;
      }

      ec.clear();
      fs::rename(entry.staged, entry.destination, ec);

      if (ec) {
        error = "Cannot install " + entry.destination.string() + ": " + ec.message();

        success = false;
        break;
      }

      entry.installed = true;

      if (ShouldFail(injection, TransactionFailurePoint::AfterInstall, i)) {
        error = "Injected failure after install.";
        success = false;
        break;
      }
    }
  }

  // Phase 3: Roll back in reverse order on failure.
  if (!success) {
    bool rollbackOk = true;

    for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
      bool destinationReady = true;

      if (it->installed) {
        ec.clear();
        fs::remove(it->destination, ec);

        if (ec) {
          rollbackOk = false;
          destinationReady = false;
        }
      }

      if (it->movedOriginal && destinationReady) {
        ec.clear();
        fs::rename(it->backup, it->destination, ec);

        if (ec) rollbackOk = false;
      } else if (it->movedOriginal) {
        rollbackOk = false;
      }
    }

    if (!rollbackOk) {
      error += "; rollback incomplete; backups retained at " + work.string();
      return false;
    }
  }

  // Phase 4: Remove staging files.
  ec.clear();
  fs::remove_all(work, ec);

  if (ec && success) {
    error = "Committed, but could not remove staging directory: " + ec.message();
  } else if (ec) {
    error += "; could not remove staging directory: " + ec.message();
  }

  return success;
}

}  // namespace Levye
