#pragma once

namespace Levye {
/**
 * @brief Services provided by the Levye host to reloadable game code.
 *
 * HostServices creates a stable boundary that allows dynamically loaded
 * game modules to use host-owned systems without linking another copy of
 * LevyeKit into the game library.
 *
 * Additional services such as assets, audio and input actions can be
 * exposed through this structure as LevyeKit grows.
 */
struct HostServices {
  /**
   * @brief Writes an informational message through the host.
   *
   * @param message Null-terminated message to write.
   */
  void (*LogInfo)(const char *message) = nullptr;

  /**
   * @brief Writes a warning message through the host.
   *
   * @param message Null-terminated message to write.
   */
  void (*LogWarning)(const char *message) = nullptr;

  /**
   * @brief Writes an error message through the host.
   *
   * @param message Null-terminated message to write.
   */
  void (*LogError)(const char *message) = nullptr;
};
} // namespace Levye