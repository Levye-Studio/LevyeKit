#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "FileTransaction.hpp"

namespace Levye {

class ProjectIDE {
 public:
  static bool Prepare(const std::filesystem::path& projectDirectory,
                      const std::filesystem::path& templateDirectory, std::string_view target,
                      std::vector<PendingFile>& changes,
                      std::vector<std::filesystem::path>& preserved, std::string& error);
};
}  // namespace Levye
