#pragma once

#include <filesystem>
#include <string_view>

namespace Levye {

/**
 * @brief Provides platform-specific shared-library naming utilities.
 *
 * SharedLibrary centralizes differences in dynamic-library file extensions
 * so the rest of LevyeKit does not need platform-specific filename checks.
 */
class SharedLibrary {
public:
  /**
   * @brief Returns the shared-library extension for the current platform.
   *
   * @return Platform extension including the leading period.
   */
  static constexpr std::string_view GetExtension() {
#if defined(_WIN32)
    return ".dll";
#elif defined(__APPLE__)
    return ".dylib";
#elif defined(__linux__)
    return ".so";
#else
    return "";
#endif
  }

  /**
   * @brief Creates a shared-library filename for the current platform.
   *
   * LevyeKit game modules intentionally do not use the Unix "lib" prefix,
   * so a module named "Game" becomes Game.dylib, Game.so, or Game.dll.
   *
   * @param name Base name of the shared library.
   * @return Platform-specific shared-library filename.
   */
  static std::filesystem::path MakeFilename(std::string_view name);
};

} // namespace Levye