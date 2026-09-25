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

  // -----------------------------------------------------------------
  // Audio
  // -----------------------------------------------------------------

  /**
   * @brief Loads a short sound effect through the host audio system.
   */
  AssetHandle (*LoadSound)(void *context, const char *path) = nullptr;

  /**
   * @brief Plays a previously loaded sound effect.
   */
  void (*PlaySound)(void *context, AssetHandle handle) = nullptr;

  /**
   * @brief Stops a currently playing sound effect.
   */
  void (*StopSound)(void *context, AssetHandle handle) = nullptr;

  /**
   * @brief Changes the volume of a sound effect.
   */
  void (*SetSoundVolume)(void *context, AssetHandle handle,
                         float volume) = nullptr;

  // -----------------------------------------------------------------
  // Music
  // -----------------------------------------------------------------

  /**
   * @brief Loads a streaming music track through the host audio system.
   */
  AssetHandle (*LoadMusic)(void *context, const char *path) = nullptr;

  /**
   * @brief Starts or resumes music playback.
   */
  void (*PlayMusic)(void *context, AssetHandle handle) = nullptr;

  /**
   * @brief Pauses music playback.
   */
  void (*PauseMusic)(void *context, AssetHandle handle) = nullptr;

  /**
   * @brief Resumes paused music playback.
   */
  void (*ResumeMusic)(void *context, AssetHandle handle) = nullptr;

  /**
   * @brief Stops music playback.
   */
  void (*StopMusic)(void *context, AssetHandle handle) = nullptr;

  /**
   * @brief Changes the volume of a music track.
   */
  void (*SetMusicVolume)(void *context, AssetHandle handle,
                         float volume) = nullptr;

  // -----------------------------------------------------------------
  // Shaders
  // -----------------------------------------------------------------

  /**
   * @brief Loads a shader through the host graphics system.
   *
   * Either source path may be nullptr to use raylib's default shader stage.
   */
  AssetHandle (*LoadShader)(void *context, const char *vertexPath,
                            const char *fragmentPath) = nullptr;

  /**
   * @brief Retrieves a host-owned shader.
   *
   * The returned pointer is temporary and must not be stored by game code.
   */
  const Shader *(*GetShader)(void *context, AssetHandle handle) = nullptr;

  // -----------------------------------------------------------------------------
  // Time
  // -----------------------------------------------------------------------------

  /**
   * @brief Returns the scaled frame duration in seconds.
   */
  float (*GetDeltaTime)(void *context) = nullptr;

  /**
   * @brief Returns the unscaled frame duration in seconds.
   */
  float (*GetUnscaledDeltaTime)(void *context) = nullptr;

  /**
   * @brief Returns total scaled runtime in seconds.
   */
  double (*GetTime)(void *context) = nullptr;

  /**
   * @brief Returns total unscaled runtime in seconds.
   */
  double (*GetUnscaledTime)(void *context) = nullptr;

  /**
   * @brief Changes the scale applied to game time.
   */
  void (*SetTimeScale)(void *context, float scale) = nullptr;

  /**
   * @brief Returns the current game time scale.
   */
  float (*GetTimeScale)(void *context) = nullptr;

  /**
   * @brief Pauses or resumes scaled game time.
   */
  void (*SetPaused)(void *context, bool paused) = nullptr;

  /**
   * @brief Returns whether scaled game time is paused.
   */
  bool (*IsPaused)(void *context) = nullptr;
};
} // namespace Levye