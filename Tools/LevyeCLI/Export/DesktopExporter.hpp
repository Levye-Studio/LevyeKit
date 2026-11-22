#pragma once
#include <filesystem>
#include <string_view>
namespace Levye {
/** @brief Packages a desktop Release build for the current host platform. */
class DesktopExporter {
 public:
  /** @brief Builds and exports a distributable desktop package. */
  static bool Execute(const std::filesystem::path& projectDirectory, std::string_view format);
};
}  // namespace Levye
