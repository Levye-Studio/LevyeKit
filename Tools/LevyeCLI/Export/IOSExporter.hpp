#pragma once

#include <filesystem>
#include <string_view>

namespace Levye {

/** @brief Packages a relocatable iOS source project with a local Xcode generation script. */
class IOSExporter {
 public:
  /** @brief Packages a relocatable iOS source tree for local Xcode generation. */
  static bool Execute(const std::filesystem::path& projectDirectory, std::string_view format);
};

}  // namespace Levye
