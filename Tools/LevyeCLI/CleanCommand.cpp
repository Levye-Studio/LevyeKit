#include "CleanCommand.hpp"

#include <iostream>
#include <string>

namespace {

bool RemoveDirectory(const std::filesystem::path &path) {
  if (!std::filesystem::exists(path))
    return true;

  std::error_code error;

  std::filesystem::remove_all(path, error);

  if (error) {
    std::cerr << "[Levye] Failed to remove " << path << ": " << error.message()
              << '\n';

    return false;
  }

  return true;
}

} // namespace

namespace Levye {

bool CleanCommand::Execute(const std::filesystem::path &projectDirectory,
                           BuildConfiguration configuration) {
  const std::string configurationName{ToString(configuration)};

  const std::filesystem::path buildDirectory =
      projectDirectory / "Build" / configurationName;

  const std::filesystem::path targetDirectory =
      projectDirectory / "Targets" / configurationName;

  /*
   * Build and Targets are intentionally cleaned separately. Build contains
   * CMake-generated files while Targets contains the runnable game and
   * reloadable game module.
   */
  const bool buildRemoved = RemoveDirectory(buildDirectory);

  const bool targetsRemoved = RemoveDirectory(targetDirectory);

  if (!buildRemoved || !targetsRemoved)
    return false;

  std::cout << "[Levye] Cleaned " << configurationName << " build.\n";

  return true;
}

bool CleanCommand::ExecuteAll(const std::filesystem::path &projectDirectory) {
  const std::filesystem::path buildDirectory = projectDirectory / "Build";

  const std::filesystem::path targetDirectory = projectDirectory / "Targets";

  const bool buildRemoved = RemoveDirectory(buildDirectory);

  const bool targetsRemoved = RemoveDirectory(targetDirectory);

  if (!buildRemoved || !targetsRemoved)
    return false;

  std::cout << "[Levye] Cleaned all builds.\n";

  return true;
}

} // namespace Levye