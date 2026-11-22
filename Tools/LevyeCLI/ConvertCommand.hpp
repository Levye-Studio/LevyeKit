
#pragma once

#include <filesystem>

namespace Levye {

/** @brief Converts a supported CMake C++ project into a LevyeKit project. */
class ConvertCommand {
 public:
  /** @brief Plans or applies a project conversion.
   * @return 0 for success/current, 2 for a pending check, or 1 on error.
   */
  static int Execute(const std::filesystem::path& projectDirectory,
                     const std::filesystem::path& frameworkRoot, bool dryRun, bool check);
};

}  // namespace Levye
