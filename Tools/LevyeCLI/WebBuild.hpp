#pragma once
#include <filesystem>

#include "BuildConfiguration.hpp"
namespace Levye {
/** @brief Builds a browser-ready development target without publishing an export. */
class WebBuild {
 public:
  static std::filesystem::path Output(const std::filesystem::path& project,
                                      BuildConfiguration configuration);
  static bool Execute(const std::filesystem::path& project, BuildConfiguration configuration);
};
}  // namespace Levye
