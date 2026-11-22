
#pragma once

#include <filesystem>

namespace Levye {

/** @brief Migrates supported older generated projects to the current template. */
class UpgradeCommand {
 public:
  /** @brief Plans or applies a project upgrade.
   * @return In check mode, 0 if current, 2 if changes are needed, or 1 on error.
   */
  static int Execute(const std::filesystem::path& projectDirectory,
                     const std::filesystem::path& frameworkRoot, bool dryRun, bool check);
};

}  // namespace Levye
