#include "AndroidExporter.hpp"

#include <Levye/Project/ProjectLoader.hpp>
#include <filesystem>
#include <iostream>
#include <random>
#include <regex>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#include "../BuildCommand.hpp"
#include "AndroidSigning.hpp"

namespace Levye {
namespace {

namespace fs = std::filesystem;

class ExportLock {
 public:
  explicit ExportLock(const fs::path& path) : path_(path) {
    std::error_code error;
    acquired_ = fs::create_directory(path_, error);
  }
  ~ExportLock() {
    if (acquired_) {
      std::error_code error;
      fs::remove(path_, error);
    }
  }
  [[nodiscard]] bool Acquired() const {
    return acquired_;
  }

 private:
  fs::path path_;
  bool acquired_ = false;
};

bool Publish(const fs::path& source, const fs::path& destination, std::error_code& error) {
#ifdef _WIN32
  if (!MoveFileExW(source.c_str(), destination.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    error = std::error_code(static_cast<int>(GetLastError()), std::system_category());
    return false;
  }
#else
  fs::rename(source, destination, error);
  if (error)
    return false;
#endif
  return true;
}

}  // namespace

bool AndroidExporter::Execute(const std::filesystem::path& projectDirectory,
                              std::string_view format) {
  if (format.empty()) {
    format = "apk";
  }

  if (format != "apk" && format != "aab" && format != "all") {
    std::cerr << "[Levye] Android export format '" << format << "' is not implemented yet.\n";

    return false;
  }

  const auto project = ProjectLoader::Load(projectDirectory / "levye.project");

  if (!project) {
    return false;
  }

  static const std::regex versionPattern(R"([A-Za-z0-9][A-Za-z0-9._+-]*)");
  static const std::regex targetPattern(R"([A-Za-z_][A-Za-z0-9_]*)");
  static const std::regex packagePattern(R"([A-Za-z][A-Za-z0-9_]*(\.[A-Za-z][A-Za-z0-9_]*)+)");
  if (!std::regex_match(project->androidVersionName, versionPattern) ||
      !std::regex_match(project->target, targetPattern) ||
      !std::regex_match(project->androidPackage, packagePattern) ||
      project->androidVersionCode <= 0) {
    std::cerr << "[Levye] Invalid Android package, version, or target for export.\n";
    return false;
  }

  const ExportLock lock(projectDirectory / ".levye-android-export.lock");
  if (!lock.Acquired()) {
    std::cerr << "[Levye] Another Android export is active, or a stale export lock exists.\n";
    return false;
  }

  if (!AndroidSigning::EnsureConfigured(projectDirectory)) {
    return false;
  }

  if (!BuildCommand::Execute(projectDirectory, BuildConfiguration::Release,
                             ProjectPlatform::Android, true, format)) {
    std::cerr << "[Levye] Android Release build failed.\n";
    return false;
  }

  const auto exportDirectory = projectDirectory / "Exports/Android" / project->androidVersionName;

  std::error_code error;

  for (const auto& directory :
       {projectDirectory / "Exports", projectDirectory / "Exports/Android", exportDirectory}) {
    error.clear();
    const auto status = std::filesystem::symlink_status(directory, error);
    if (error && error != std::errc::no_such_file_or_directory) {
      std::cerr << "[Levye] Cannot inspect export directory: " << error.message() << '\n';
      return false;
    }
    if (!error && std::filesystem::is_symlink(status)) {
      std::cerr << "[Levye] Export directory must not be a symbolic link: " << directory << '\n';
      return false;
    }
  }

  error.clear();
  std::filesystem::create_directories(exportDirectory, error);

  if (error) {
    std::cerr << "[Levye] Cannot create export directory: " << error.message() << '\n';
    return false;
  }

  struct Artifact {
    fs::path source;
    fs::path destination;
    fs::path staged;
    fs::path backup;
    bool published = false;
  };
  std::vector<Artifact> artifacts;
  for (const std::string extension : {"apk", "aab"}) {
    if (format != "all" && format != extension)
      continue;
    const auto filename = project->target + "-v" + project->androidVersionName + "-" +
                          std::to_string(project->androidVersionCode) + "." + extension;
    artifacts.push_back(
        {projectDirectory / "Targets/android-Release/bin" / (project->target + "." + extension),
         exportDirectory / filename,
         {},
         {},
         false});
  }

  // Keep candidates and backups together on the destination filesystem.
  fs::path stageDirectory;
  std::random_device random;

  for (int attempt = 0; attempt < 32; ++attempt) {
    auto candidate = exportDirectory / (".stage." + std::to_string(random()));
    error.clear();

    if (fs::create_directory(candidate, error)) {
      stageDirectory = candidate;
      break;
    }
    if (error && error != std::errc::file_exists)
      break;
  }

  if (stageDirectory.empty()) {
    std::cerr << "[Levye] Cannot create Android export staging directory.\n";
    return false;
  }

  const auto cleanup = [&]() {
    std::error_code cleanupError;
    fs::remove_all(stageDirectory, cleanupError);
    if (cleanupError)
      std::cerr << "[Levye] Cannot clean staging directory: " << stageDirectory << '\n';
  };
  for (auto& artifact : artifacts) {
    error.clear();
    if (!fs::is_regular_file(fs::symlink_status(artifact.source, error)) || error) {
      std::cerr << "[Levye] Missing or invalid built artifact: " << artifact.source << '\n';
      cleanup();
      return false;
    }
    artifact.staged = stageDirectory / artifact.destination.filename();
    if (!fs::copy_file(artifact.source, artifact.staged, fs::copy_options::none, error)) {
      std::cerr << "[Levye] Cannot stage artifact: " << error.message() << '\n';
      cleanup();
      return false;
    }
    error.clear();
    const auto status = fs::symlink_status(artifact.destination, error);
    if (error && error != std::errc::no_such_file_or_directory) {
      std::cerr << "[Levye] Cannot inspect destination: " << error.message() << '\n';
      cleanup();
      return false;
    }
    if (fs::exists(status)) {
      if (!fs::is_regular_file(status)) {
        std::cerr << "[Levye] Invalid export destination: " << artifact.destination << '\n';
        cleanup();
        return false;
      }
      artifact.backup = artifact.staged.string() + ".backup";
      error.clear();
      if (!fs::copy_file(artifact.destination, artifact.backup, fs::copy_options::none, error)) {
        std::cerr << "[Levye] Cannot back up existing export: " << error.message() << '\n';
        cleanup();
        return false;
      }
    }
  }

  for (auto& artifact : artifacts) {
    error.clear();
    if (!Publish(artifact.staged, artifact.destination, error)) {
      std::cerr << "[Levye] Cannot publish artifact: " << error.message() << '\n';
      bool restored = true;
      for (auto& previous : artifacts) {
        if (!previous.published)
          continue;
        std::error_code rollbackError;
        if (previous.backup.empty()) {
          fs::remove(previous.destination, rollbackError);
        } else {
          Publish(previous.backup, previous.destination, rollbackError);
        }
        if (rollbackError) {
          restored = false;
          std::cerr << "[Levye] Rollback failed: " << previous.destination << ": "
                    << rollbackError.message() << '\n';
        }
      }
      if (restored)
        cleanup();
      else
        std::cerr << "[Levye] Recovery files retained in: " << stageDirectory << '\n';
      return false;
    }
    artifact.published = true;
  }
  cleanup();
  for (const auto& artifact : artifacts) {
    std::cout << "[Levye] Android " << (artifact.destination.extension() == ".aab" ? "AAB" : "APK")
              << " exported:\n"
              << artifact.destination << '\n';
  }
  return true;
}

}  // namespace Levye
