#include "CleanCommand.hpp"

#include <Levye/Project/ProjectLoader.hpp>
#include <iostream>
#include <string>

namespace {

bool RemoveDirectory(const std::filesystem::path& path) {
  std::error_code error;

  // remove_all also succeeds for a missing directory. Use its error_code
  // overload so locked DLLs and inaccessible paths produce a CLI diagnostic.
  std::filesystem::remove_all(path, error);

  if (error) {
    std::cerr << "[Levye] Failed to remove " << path << ": " << error.message() << '\n';

    return false;
  }

  return true;
}

}  // namespace

namespace Levye {

bool CleanCommand::Execute(const std::filesystem::path& projectDirectory,
                           BuildConfiguration configuration, ProjectPlatform platform) {
  const std::string configurationName{ToString(configuration)};
  const auto project = ProjectLoader::Load(projectDirectory / "levye.project");
  if (!project) return false;
  platform = ResolveProjectPlatform(platform, project->defaultPlatform);
  const std::string prefix = platform == ProjectPlatform::Android ? "android-"
                             : platform == ProjectPlatform::IOS   ? "ios-"
                                                                  : "";

  const std::filesystem::path buildDirectory =
      projectDirectory / "Build" / (prefix + configurationName);

  const std::filesystem::path targetDirectory =
      projectDirectory / "Targets" / (prefix + configurationName);

  /*
   * Build and Targets are intentionally cleaned separately. Build contains
   * CMake-generated files while Targets contains the runnable game and
   * reloadable game module.
   */
  const bool buildRemoved = RemoveDirectory(buildDirectory);

  const bool targetsRemoved = RemoveDirectory(targetDirectory);

  if (!buildRemoved || !targetsRemoved) return false;

  std::cout << "[Levye] Cleaned " << configurationName << " build.\n";

  return true;
}

bool CleanCommand::ExecuteAll(const std::filesystem::path& projectDirectory) {
  const std::filesystem::path buildDirectory = projectDirectory / "Build";

  const std::filesystem::path targetDirectory = projectDirectory / "Targets";

  const bool buildRemoved = RemoveDirectory(buildDirectory);

  const bool targetsRemoved = RemoveDirectory(targetDirectory);

  if (!buildRemoved || !targetsRemoved) return false;

  std::cout << "[Levye] Cleaned all builds.\n";

  return true;
}

}  // namespace Levye
