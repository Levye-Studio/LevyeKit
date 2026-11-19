#pragma once

#include <filesystem>
#include <string_view>

namespace Levye {

/** @brief Inspects a project or updates its persistent default platform. */
class ProjectCommand {
 public:
  /** @brief Runs the project command; an empty value only displays information. */
  static bool Execute(const std::filesystem::path& projectDirectory,
                      std::string_view requestedDefault = {});
};

}  // namespace Levye
