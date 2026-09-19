#pragma once

#include <Levye/Assets/AssetHandle.hpp>
#include <raylib.h>

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

  // -----------------------------------------------------------------
  // Textures
  // -----------------------------------------------------------------

  /**
   * @brief Loads a texture through the host-owned texture manager.
   *
   * Loading the same path more than once returns the existing handle.
   *
   * @param context Host-owned service context.
   * @param path Path to the texture file.
   * @return Handle identifying the texture.
   */
  AssetHandle (*LoadTexture)(void *context, const char *path) = nullptr;

  /**
   * @brief Returns a host-owned texture for drawing.
   *
   * The returned texture remains owned by LevyeKit. Game code may use it for
   * rendering but must never call UnloadTexture() on it.
   *
   * @param context Host-owned service context.
   * @param handle Texture asset handle.
   * @return Pointer to the texture, or nullptr when the handle is invalid.
   *
   * @warning The returned pointer must not be stored permanently. Retrieve it
   * again when needed so future asset hot reloads can replace the resource.
   */
  const Texture2D *(*GetTexture)(void *context, AssetHandle handle) = nullptr;
};
} // namespace Levye