#pragma once

#include <filesystem>
#include <string_view>

namespace Levye {

/**
 * @brief Enables optional modules in existing LevyeKit projects.
 */
class AddCommand {
 public:
  /**
   * @brief Enables a module in the specified project.
   *
   * @return true on success, including when already enabled.
   */
  static bool Execute(const std::filesystem::path& projectDirectory,
                      std::string_view module);
};

}  // namespace Levye