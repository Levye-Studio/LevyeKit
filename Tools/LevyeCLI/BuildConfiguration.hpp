#pragma once

#include <string_view>

namespace Levye {

/**
 * @brief Build configurations supported by the Levye CLI.
 */
enum class BuildConfiguration { Debug, Release };

/**
 * @brief Returns the CMake build-type name for a build configuration.
 *
 * @param configuration Build configuration to convert.
 * @return CMake-compatible build-type name.
 */
inline constexpr std::string_view ToString(BuildConfiguration configuration) {
  switch (configuration) {
  case BuildConfiguration::Debug:
    return "Debug";

  case BuildConfiguration::Release:
    return "Release";
  }

  return "Debug";
}

} // namespace Levye