#pragma once

#include <Levye/Assets/AssetHandle.hpp>

#include <raylib.h>

namespace Levye {

/**
 * @brief Services exposed by the LevyeKit host to reloadable game code.
 *
 * HostServices forms part of the stable boundary between the application
 * host and the reloadable game module.
 *
 * Game code should use these callbacks to access host-owned systems instead
 * of directly accessing their manager implementations.
 */
struct HostServices {
  /**
   * @brief Opaque host-owned context passed to service callbacks.
   *
   * Game code must not cast, modify, or attempt to own this pointer.
   */
  void *context = nullptr;

  // ---------------------------------------------------------------------
  // Logging
  // ---------------------------------------------------------------------

  /**
   * @brief Writes an informational message to the host logger.
   */
  void (*LogInfo)(void *context, const char *message) = nullptr;

  /**
   * @brief Writes a warning message to the host logger.
   */
  void (*LogWarning)(void *context, const char *message) = nullptr;

  /**
   * @brief Writes an error message to the host logger.
   */
  void (*LogError)(void *context, const char *message) = nullptr;

  // ---------------------------------------------------------------------
  // Input
  // ---------------------------------------------------------------------

  /**
   * @brief Returns whether an input action is currently active.
   */
  bool (*IsActionDown)(void *context, const char *action) = nullptr;

  /**
   * @brief Returns whether an input action became active this frame.
   */
  bool (*IsActionPressed)(void *context, const char *action) = nullptr;

  /**
   * @brief Returns whether an input action was released this frame.
   */
  bool (*IsActionReleased)(void *context, const char *action) = nullptr;

  /**
   * @brief Returns the current value of a named input axis.
   */
  float (*GetAxis)(void *context, const char *axis) = nullptr;

  /**
   * @brief Associates a keyboard key with an input action.
   *
   * @param context Host-owned service context.
   * @param action Name of the action to bind.
   * @param key raylib keyboard key code.
   */
  void (*BindKey)(void *context, const char *action, int key) = nullptr;

  /**
   * @brief Associates two keyboard keys with a directional axis.
   *
   * @param context Host-owned service context.
   * @param action Name of the axis to bind.
   * @param negativeKey Key representing the negative direction.
   * @param positiveKey Key representing the positive direction.
   */
  void (*BindKeyAxis)(void *context, const char *action, int negativeKey,
                      int positiveKey) = nullptr;

  // ---------------------------------------------------------------------
  // Screens
  // ---------------------------------------------------------------------

  /**
   * @brief Changes the active logical game screen.
   */
  void (*SetScreen)(void *context, const char *screen) = nullptr;

  /**
   * @brief Returns whether the specified logical screen is active.
   */
  bool (*IsScreen)(void *context, const char *screen) = nullptr;

  // ---------------------------------------------------------------------
  // Textures
  // ---------------------------------------------------------------------

  /**
   * @brief Loads or retrieves a texture managed by the host.
   */
  AssetHandle (*LoadTexture)(void *context, const char *path) = nullptr;

  /**
   * @brief Returns the raylib texture represented by a texture handle.
   */
  const Texture2D *(*GetTexture)(void *context, AssetHandle handle) = nullptr;

  // ---------------------------------------------------------------------
  // Sound
  // ---------------------------------------------------------------------

  /**
   * @brief Loads or retrieves a sound managed by the host.
   */
  AssetHandle (*LoadSound)(void *context, const char *path) = nullptr;

  /** @brief Starts playback of a loaded sound. */
  void (*PlaySound)(void *context, AssetHandle handle) = nullptr;

  /** @brief Stops playback of a loaded sound. */
  void (*StopSound)(void *context, AssetHandle handle) = nullptr;

  /** @brief Changes the playback volume of a loaded sound. */
  void (*SetSoundVolume)(void *context, AssetHandle handle,
                         float volume) = nullptr;

  // ---------------------------------------------------------------------
  // Music
  // ---------------------------------------------------------------------

  /**
   * @brief Loads or retrieves a music stream managed by the host.
   */
  AssetHandle (*LoadMusic)(void *context, const char *path) = nullptr;

  /** @brief Starts playback of a music stream. */
  void (*PlayMusic)(void *context, AssetHandle handle) = nullptr;

  /** @brief Pauses a playing music stream. */
  void (*PauseMusic)(void *context, AssetHandle handle) = nullptr;

  /** @brief Resumes a paused music stream. */
  void (*ResumeMusic)(void *context, AssetHandle handle) = nullptr;

  /** @brief Stops a music stream. */
  void (*StopMusic)(void *context, AssetHandle handle) = nullptr;

  /** @brief Changes the playback volume of a music stream. */
  void (*SetMusicVolume)(void *context, AssetHandle handle,
                         float volume) = nullptr;

  // ---------------------------------------------------------------------
  // Shaders
  // ---------------------------------------------------------------------

  /**
   * @brief Loads or retrieves a shader managed by the host.
   *
   * Either shader path may be null when only one shader stage is required.
   */
  AssetHandle (*LoadShader)(void *context, const char *vertexPath,
                            const char *fragmentPath) = nullptr;

  /**
   * @brief Returns the raylib shader represented by a shader handle.
   */
  const Shader *(*GetShader)(void *context, AssetHandle handle) = nullptr;

  // ---------------------------------------------------------------------
  // Fonts
  // ---------------------------------------------------------------------

  /**
   * @brief Loads or retrieves a font managed by the host.
   */
  AssetHandle (*LoadFont)(void *context, const char *path,
                          int fontSize) = nullptr;

  /**
   * @brief Returns the raylib font represented by a font handle.
   */
  const Font *(*GetFont)(void *context, AssetHandle handle) = nullptr;

  // ---------------------------------------------------------------------
  // Time
  // ---------------------------------------------------------------------

  /** @brief Returns scaled frame delta time in seconds. */
  float (*GetDeltaTime)(void *context) = nullptr;

  /** @brief Returns unscaled frame delta time in seconds. */
  float (*GetUnscaledDeltaTime)(void *context) = nullptr;

  /** @brief Returns accumulated scaled game time in seconds. */
  double (*GetTime)(void *context) = nullptr;

  /** @brief Returns accumulated unscaled game time in seconds. */
  double (*GetUnscaledTime)(void *context) = nullptr;

  /**
   * @brief Changes the scale applied to game time.
   */
  void (*SetTimeScale)(void *context, float scale) = nullptr;

  /** @brief Returns the current game-time scale. */
  float (*GetTimeScale)(void *context) = nullptr;

  /**
   * @brief Changes the paused state of scaled game time.
   */
  void (*SetPaused)(void *context, bool paused) = nullptr;

  /** @brief Returns whether scaled game time is paused. */
  bool (*IsPaused)(void *context) = nullptr;

  /**
   * @brief Returns the interpolation factor for fixed-step rendering.
   */
  float (*GetInterpolationAlpha)(void *context) = nullptr;
};

} // namespace Levye