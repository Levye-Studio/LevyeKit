#pragma once

#include <string_view>

namespace Levye {

/**
 * @brief Build configurations supported by the Levye CLI.
 */
enum class BuildConfiguration { Debug, Release };

/** @brief Requested project platform; ProjectDefault reads levye.project. */
enum class ProjectPlatform { ProjectDefault, Desktop, Android, IOS, Web };

/** @brief Resolves a one-command override against the manifest default. */
inline constexpr ProjectPlatform ResolveProjectPlatform(ProjectPlatform requested,
                                                        std::string_view storedDefault) {
  if (requested != ProjectPlatform::ProjectDefault)
    return requested;
  return storedDefault == "android" ? ProjectPlatform::Android
         : storedDefault == "ios"   ? ProjectPlatform::IOS
         : storedDefault == "web"   ? ProjectPlatform::Web
                                    : ProjectPlatform::Desktop;
}

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

}  // namespace Levye
