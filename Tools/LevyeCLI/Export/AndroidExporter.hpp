#pragma once

#include <filesystem>
#include <string_view>

namespace Levye {

/** @brief Exports production-signed Android APK and App Bundle artifacts. */
class AndroidExporter {
 public:
  /** @brief Builds and exports the requested Android package format. */
  static bool Execute(const std::filesystem::path& projectDirectory, std::string_view format);
};

}  // namespace Levye
