#pragma once
#include <filesystem>

#include "BuildConfiguration.hpp"
namespace Levye {
/** @brief Builds a browser-ready development target without publishing an export. */
class WebBuild {
 public:
  /** @brief Returns the CMake build directory for a Web configuration. */
  static std::filesystem::path BuildDirectory(const std::filesystem::path& project,
                                              BuildConfiguration configuration);

  /** @brief Returns the browser-ready output directory. */
  static std::filesystem::path Output(const std::filesystem::path& project,
                                      BuildConfiguration configuration);

  /** @brief Configures and builds the Web target. */
  static bool Execute(const std::filesystem::path& project, BuildConfiguration configuration);
};
}  // namespace Levye
