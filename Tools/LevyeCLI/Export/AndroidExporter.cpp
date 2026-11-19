#include "AndroidExporter.hpp"

#include <Levye/Project/ProjectLoader.hpp>
#include <filesystem>
#include <iostream>
#include <random>
#include <regex>
#include <string>

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
  [[nodiscard]] bool Acquired() const { return acquired_; }

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
  if (error) return false;
#endif
  return true;
}

}  // namespace

bool AndroidExporter::Execute(const std::filesystem::path& projectDirectory,
                              std::string_view format) {
  if (format.empty()) {
    format = "apk";
  }

  if (format != "apk") {
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
                             ProjectPlatform::Android, true)) {
    std::cerr << "[Levye] Android Release build failed.\n";
    return false;
  }

  const auto source = projectDirectory / "Targets/android-Release/bin" / (project->target + ".apk");

  if (!std::filesystem::is_regular_file(source)) {
    std::cerr << "[Levye] Built APK not found: " << source << '\n';
    return false;
  }

  const auto exportDirectory = projectDirectory / "Exports/Android" / project->androidVersionName;

  const auto destination =
      exportDirectory / (project->target + "-v" + project->androidVersionName + "-" +
                         std::to_string(project->androidVersionCode) + ".apk");

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

  // A unique, no-overwrite stage file prevents stale paths from redirecting a copy.
  std::filesystem::path temporary;
  std::random_device random;
  bool staged = false;
  for (int attempt = 0; attempt < 32; ++attempt) {
    temporary = std::filesystem::path(destination.string() + ".tmp." + std::to_string(random()));
    error.clear();
    staged =
        std::filesystem::copy_file(source, temporary, std::filesystem::copy_options::none, error);
    if (staged || error != std::errc::file_exists) break;
  }

  if (!staged) {
    std::cerr << "[Levye] Cannot stage APK: " << error.message() << '\n';
    return false;
  }

  if (!Publish(temporary, destination, error)) {
    std::error_code cleanupError;
    std::filesystem::remove(temporary, cleanupError);
    std::cerr << "[Levye] Cannot publish APK: " << error.message() << '\n';
    return false;
  }

  std::cout << "[Levye] Android APK exported:\n" << destination << '\n';

  return true;
}

}  // namespace Levye
