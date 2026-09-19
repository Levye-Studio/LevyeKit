#include "AudioManager.hpp"

#include <algorithm>
#include <iostream>

namespace Levye {
AudioManager::~AudioManager() { Clear(); }

AssetHandle AudioManager::LoadSound(const std::string &path) {
  const auto existing = m_PathLookup.find(path);

  if (existing != m_PathLookup.end())
    return existing->second;

  Sound sound = ::LoadSound(path.c_str());

  if (!IsSoundValid(sound)) {
    std::cerr << "[LevyeKit] Failed to load sound: " << path << '\n';

    return {};
  }

  const AssetHandle handle{.id = m_NextHandle++};

  m_Sounds.emplace(handle.id, SoundAsset{.sound = sound, .path = path});

  m_PathLookup.emplace(path, handle);

  std::cout << "[LevyeKit] Loaded sound: " << path << '\n';

  return handle;
}

void AudioManager::PlaySound(AssetHandle handle) {
  const auto iterator = m_Sounds.find(handle.id);

  if (iterator == m_Sounds.end())
    return;

  ::PlaySound(iterator->second.sound);
}

void AudioManager::StopSound(AssetHandle handle) {
  const auto iterator = m_Sounds.find(handle.id);

  if (iterator == m_Sounds.end())
    return;

  ::StopSound(iterator->second.sound);
}

void AudioManager::SetSoundVolume(AssetHandle handle, float volume) {
  const auto iterator = m_Sounds.find(handle.id);

  if (iterator == m_Sounds.end())
    return;

  volume = std::clamp(volume, 0.0f, 1.0f);

  ::SetSoundVolume(iterator->second.sound, volume);
}

void AudioManager::UnloadSound(AssetHandle handle) {
  const auto iterator = m_Sounds.find(handle.id);

  if (iterator == m_Sounds.end())
    return;

  ::UnloadSound(iterator->second.sound);

  m_PathLookup.erase(iterator->second.path);

  m_Sounds.erase(iterator);
}

AssetHandle AudioManager::LoadMusic(const std::string &path) {
  const auto existing = m_MusicPathLookup.find(path);

  if (existing != m_MusicPathLookup.end())
    return existing->second;

  Music music = ::LoadMusicStream(path.c_str());

  if (!IsMusicValid(music)) {
    std::cerr << "[LevyeKit] Failed to load music: " << path << '\n';

    return {};
  }

  const AssetHandle handle{.id = m_NextHandle++};

  m_Music.emplace(handle.id, MusicAsset{.music = music, .path = path});

  m_MusicPathLookup.emplace(path, handle);

  std::cout << "[LevyeKit] Loaded music: " << path << '\n';

  return handle;
}

void AudioManager::PlayMusic(AssetHandle handle) {
  const auto iterator = m_Music.find(handle.id);

  if (iterator == m_Music.end())
    return;

  ::PlayMusicStream(iterator->second.music);
}

void AudioManager::PauseMusic(AssetHandle handle) {
  const auto iterator = m_Music.find(handle.id);

  if (iterator == m_Music.end())
    return;

  ::PauseMusicStream(iterator->second.music);
}

void AudioManager::ResumeMusic(AssetHandle handle) {
  const auto iterator = m_Music.find(handle.id);

  if (iterator == m_Music.end())
    return;

  ::ResumeMusicStream(iterator->second.music);
}

void AudioManager::StopMusic(AssetHandle handle) {
  const auto iterator = m_Music.find(handle.id);

  if (iterator == m_Music.end())
    return;

  ::StopMusicStream(iterator->second.music);
}

void AudioManager::SetMusicVolume(AssetHandle handle, float volume) {
  const auto iterator = m_Music.find(handle.id);

  if (iterator == m_Music.end())
    return;

  volume = std::clamp(volume, 0.0f, 1.0f);

  ::SetMusicVolume(iterator->second.music, volume);
}

void AudioManager::Update() {
  for (auto &[id, asset] : m_Music) {
    (void)id;

    if (::IsMusicStreamPlaying(asset.music)) {
      ::UpdateMusicStream(asset.music);
    }
  }
}

void AudioManager::UnloadMusic(AssetHandle handle) {
  const auto iterator = m_Music.find(handle.id);

  if (iterator == m_Music.end())
    return;

  ::UnloadMusicStream(iterator->second.music);

  m_MusicPathLookup.erase(iterator->second.path);

  m_Music.erase(iterator);
}

void AudioManager::Clear() {

  for (auto &[id, asset] : m_Music) {
    (void)id;

    ::UnloadMusicStream(asset.music);
  }

  m_Music.clear();
  m_MusicPathLookup.clear();

  for (auto &[id, asset] : m_Sounds) {
    (void)id;

    ::UnloadSound(asset.sound);
  }

  m_Sounds.clear();
  m_PathLookup.clear();
}
} // namespace Levye