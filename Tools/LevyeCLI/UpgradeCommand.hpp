
#pragma once

#include <filesystem>

namespace Levye {

class UpgradeCommand {
 public:
  // --check: 0 = current, 2 = upgrade needed, 1 = unsupported/error.
  static int Execute(const std::filesystem::path& projectDirectory,
                     const std::filesystem::path& frameworkRoot, bool dryRun, bool check);
};

}  // namespace Levye
