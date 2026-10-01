#pragma once

#include <Levye/Core/Services.hpp>

namespace Levye {

/**
 * @brief Provides access to host-owned audio resources and playback.
 *
 * Audio forwards sound and music operations to the host through
 * HostServices. Audio resources remain owned by the host so their handles
 * can safely remain in persistent game state across compatible code reloads.
 */
class Audio {
public:
  // ---------------------------------------------------------------------
  // Sound
  // ---------------------------------------------------------------------

  /**
   * @brief Loads or retrieves a sound effect.
   *
   * Relative paths are resolved against the project's asset directory.
   *
   * @param path Path to the sound asset.
   * @return Handle representing the sound, or an invalid handle on failure.
   */
  static AssetHandle LoadSound(const char *path) {
    const HostServices *host = Services::Get();

    if (!host || !host->LoadSound || !path) {
      return {};
    }

    return host->LoadSound(host->context, path);
  }

  /**
   * @brief Starts playback of a loaded sound effect.
   *
   * @param handle Handle returned by LoadSound().
   */
  static void PlaySound(AssetHandle handle) {
    const HostServices *host = Services::Get();

    if (!host || !host->PlaySound)
      return;

    host->PlaySound(host->context, handle);
  }

  /**
   * @brief Stops playback of a loaded sound effect.
   *
   * @param handle Sound handle to stop.
   */
  static void StopSound(AssetHandle handle) {
    const HostServices *host = Services::Get();

    if (!host || !host->StopSound)
      return;

    host->StopSound(host->context, handle);
  }

  /**
   * @brief Changes the volume of a loaded sound effect.
   *
   * @param handle Sound handle to modify.
   * @param volume New playback volume.
   */
  static void SetSoundVolume(AssetHandle handle, float volume) {
    const HostServices *host = Services::Get();

    if (!host || !host->SetSoundVolume)
      return;

    host->SetSoundVolume(host->context, handle, volume);
  }

  // ---------------------------------------------------------------------
  // Music
  // ---------------------------------------------------------------------

  /**
   * @brief Loads or retrieves a music stream.
   *
   * Relative paths are resolved against the project's asset directory.
   *
   * @param path Path to the music asset.
   * @return Handle representing the music stream, or an invalid handle on
   * failure.
   */
  static AssetHandle LoadMusic(const char *path) {
    const HostServices *host = Services::Get();

    if (!host || !host->LoadMusic || !path) {
      return {};
    }

    return host->LoadMusic(host->context, path);
  }

  /**
   * @brief Starts playback of a loaded music stream.
   *
   * @param handle Music handle to play.
   */
  static void PlayMusic(AssetHandle handle) {
    const HostServices *host = Services::Get();

    if (!host || !host->PlayMusic)
      return;

    host->PlayMusic(host->context, handle);
  }

  /**
   * @brief Pauses a playing music stream.
   *
   * @param handle Music handle to pause.
   */
  static void PauseMusic(AssetHandle handle) {
    const HostServices *host = Services::Get();

    if (!host || !host->PauseMusic)
      return;

    host->PauseMusic(host->context, handle);
  }

  /**
   * @brief Resumes a paused music stream.
   *
   * @param handle Music handle to resume.
   */
  static void ResumeMusic(AssetHandle handle) {
    const HostServices *host = Services::Get();

    if (!host || !host->ResumeMusic)
      return;

    host->ResumeMusic(host->context, handle);
  }

  /**
   * @brief Stops playback of a music stream.
   *
   * @param handle Music handle to stop.
   */
  static void StopMusic(AssetHandle handle) {
    const HostServices *host = Services::Get();

    if (!host || !host->StopMusic)
      return;

    host->StopMusic(host->context, handle);
  }

  /**
   * @brief Changes the volume of a music stream.
   *
   * @param handle Music handle to modify.
   * @param volume New playback volume.
   */
  static void SetMusicVolume(AssetHandle handle, float volume) {
    const HostServices *host = Services::Get();

    if (!host || !host->SetMusicVolume)
      return;

    host->SetMusicVolume(host->context, handle, volume);
  }
};

} // namespace Levye