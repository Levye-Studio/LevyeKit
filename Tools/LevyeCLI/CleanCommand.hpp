#pragma once

#include "BuildConfiguration.hpp"

#include <filesystem>

namespace Levye {

/**
 * @brief Removes generated build files for a LevyeKit project.
 *
 * CleanCommand removes CMake build files and compiled runtime outputs without
 * modifying project source files or assets.
 */
class CleanCommand {
public:
  /**
   * @brief Cleans one build configuration.
   *
   * Removes the matching directories from Build and Targets.
   *
   * @param projectDirectory Root directory of the LevyeKit project.
   * @param configuration Build configuration to clean.
   * @return True when the generated files were removed successfully.
   */
  static bool Execute(const std::filesystem::path &projectDirectory,
                      BuildConfiguration configuration);

  /**
   * @brief Cleans all build configurations.
   *
   * Removes the complete Build and Targets directories.
   *
   * @param projectDirectory Root directory of the LevyeKit project.
   * @return True when the generated files were removed successfully.
   */
  static bool ExecuteAll(const std::filesystem::path &projectDirectory);
};

} // namespace Levye