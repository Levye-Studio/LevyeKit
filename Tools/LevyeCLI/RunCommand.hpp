#pragma once

#include <filesystem>

namespace Levye {

/**
 * @brief Runs an existing build of a LevyeKit game project.
 *
 * RunCommand locates the project's compiled host executable and launches it.
 * It does not configure or build the project.
 */
class RunCommand {
public:
  /**
   * @brief Runs the Debug build of a LevyeKit project.
   *
   * The project name is read from levye.project and used to locate the
   * executable inside Targets/Debug/bin.
   *
   * @param projectDirectory Root directory containing levye.project.
   * @return True when the game was launched and exited successfully.
   */
  static bool Execute(const std::filesystem::path &projectDirectory);
};

} // namespace Levye