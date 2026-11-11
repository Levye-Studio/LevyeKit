
#pragma once

#include <filesystem>

namespace Levye {

class ConvertCommand {
 public:
  // Returns 0 for success/current, 2 for a pending --check, and 1 for errors.
  static int Execute(const std::filesystem::path& projectDirectory,
                     const std::filesystem::path& frameworkRoot, bool dryRun, bool check);
};

}  // namespace Levye
