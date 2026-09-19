#pragma once

namespace Levye {
/**
 * @brief Services provided by the Levye host to reloadable game code.
 */
struct HostServices {
  void *context = nullptr;
  // -----------------------------------------------------------------
  // Logging
  // -----------------------------------------------------------------

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

  // -----------------------------------------------------------------
  // Input Actions
  // -----------------------------------------------------------------

  /**
   * @brief Returns whether a named input action is currently held.
   */
  bool (*IsActionDown)(void *context, const char *action) = nullptr;

  /**
   * @brief Returns whether a named input action was pressed this frame.
   */
  bool (*IsActionPressed)(void *context, const char *action) = nullptr;

  /**
   * @brief Returns whether a named input action was released this frame.
   */
  bool (*IsActionReleased)(void *context, const char *action) = nullptr;

  // -----------------------------------------------------------------
  // Input Axes
  // -----------------------------------------------------------------

  /**
   * @brief Returns the current value of a named input axis.
   *
   * @return Value between -1 and +1.
   */
  float (*GetAxis)(void *context, const char *axis) = nullptr;

  // -----------------------------------------------------------------
  // Screens
  // -----------------------------------------------------------------

  /**
   * @brief Changes the currently active logical screen.
   *
   * @param context Host-owned service context.
   * @param screen Name of the screen to activate.
   */
  void (*SetScreen)(void *context, const char *screen) = nullptr;

  /**
   * @brief Returns whether a logical screen is currently active.
   *
   * @param context Host-owned service context.
   * @param screen Screen name to test.
   */
  bool (*IsScreen)(void *context, const char *screen) = nullptr;
};
} // namespace Levye