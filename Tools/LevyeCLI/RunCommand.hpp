#pragma once

#include <filesystem>

#include "BuildConfiguration.hpp"

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
   * @brief Runs an existing build of a LevyeKit project.
   *
   * @param projectDirectory Root directory of the project.
   * @param configuration Build configuration to run.
   * @return True when the application launches and exits successfully.
   */
  static bool Execute(const std::filesystem::path& projectDirectory,
                      BuildConfiguration configuration,
                      ProjectPlatform platform = ProjectPlatform::ProjectDefault,
                      unsigned short port = 8080);
};

}  // namespace Levye
