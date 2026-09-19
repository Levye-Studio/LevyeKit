#pragma once

#include <Levye/Assets/AssetHandle.hpp>

#include <raylib.h>

#include <cstdint>
#include <string>
#include <unordered_map>

namespace Levye {
/**
 * @brief Owns short sound effects used by the game.
 *
 * AudioManager lives in the host so loaded audio resources remain valid
 * when reloadable game code is replaced.
 *
 * Game code refers to sounds using AssetHandle rather than owning Sound
 * objects directly.
 */
class AudioManager {
public:
  AudioManager() = default;

  /**
   * @brief Releases every sound still owned by the manager.
   */
  ~AudioManager();

  AudioManager(const AudioManager &) = delete;
  AudioManager &operator=(const AudioManager &) = delete;

  /**
   * @brief Loads a sound effect from disk.
   *
   * Loading the same path more than once returns the existing handle.
   *
   * @param path Path to the sound file.
   * @return Handle to the sound, or an invalid handle on failure.
   */
  AssetHandle LoadSound(const std::string &path);

  /**
   * @brief Plays a loaded sound effect.
   *
   * @param handle Sound handle returned by LoadSound().
   */
  void PlaySound(AssetHandle handle);

  /**
   * @brief Stops a currently playing sound effect.
   *
   * @param handle Sound handle returned by LoadSound().
   */
  void StopSound(AssetHandle handle);

  /**
   * @brief Changes the volume of a loaded sound.
   *
   * @param handle Sound handle.
   * @param volume Volume from 0.0 to 1.0.
   */
  void SetSoundVolume(AssetHandle handle, float volume);

  /**
   * @brief Unloads a sound resource.
   *
   * @param handle Sound handle to unload.
   */
  void UnloadSound(AssetHandle handle);

  /**
   * @brief Loads a streaming music track from disk.
   *
   * Loading the same path more than once returns the existing handle.
   *
   * @param path Path to the music file.
   * @return Handle to the music track, or an invalid handle on failure.
   */
  AssetHandle LoadMusic(const std::string &path);

  /**
   * @brief Starts or resumes playback of a music track.
   *
   * @param handle Music handle returned by LoadMusic().
   */
  void PlayMusic(AssetHandle handle);

  /**
   * @brief Pauses a playing music track.
   *
   * @param handle Music handle returned by LoadMusic().
   */
  void PauseMusic(AssetHandle handle);

  /**
   * @brief Resumes a paused music track.
   *
   * @param handle Music handle returned by LoadMusic().
   */
  void ResumeMusic(AssetHandle handle);

  /**
   * @brief Stops a music track and resets its playback position.
   *
   * @param handle Music handle returned by LoadMusic().
   */
  void StopMusic(AssetHandle handle);

  /**
   * @brief Changes the volume of a music track.
   *
   * @param handle Music handle.
   * @param volume Volume from 0.0 to 1.0.
   */
  void SetMusicVolume(AssetHandle handle, float volume);

  /**
   * @brief Updates every loaded music stream.
   *
   * Must be called once per frame while the audio device is active.
   */
  void Update();

  /**
   * @brief Unloads a music resource.
   *
   * @param handle Music handle to unload.
   */
  void UnloadMusic(AssetHandle handle);

  /**
   * @brief Releases every sound owned by the manager.
   */
  void Clear();

private:
  struct SoundAsset {
    Sound sound{};
    std::string path;
  };

  struct MusicAsset {
    Music music{};
    std::string path;
  };

  std::unordered_map<std::uint64_t, SoundAsset> m_Sounds;

  std::unordered_map<std::string, AssetHandle> m_PathLookup;

  std::unordered_map<std::uint64_t, MusicAsset> m_Music;

  std::unordered_map<std::string, AssetHandle> m_MusicPathLookup;

  std::uint64_t m_NextHandle = 1;
};
} // namespace Levye