#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "FileTransaction.hpp"

namespace Levye {

/** @brief Prepares editor configuration for a generated project. */
class ProjectIDE {
 public:
  /** @brief Plans missing IDE files while preserving existing custom files. */
  static bool Prepare(const std::filesystem::path& projectDirectory,
                      const std::filesystem::path& templateDirectory, std::string_view target,
                      std::vector<PendingFile>& changes,
                      std::vector<std::filesystem::path>& preserved, std::string& error);
};
}  // namespace Levye
