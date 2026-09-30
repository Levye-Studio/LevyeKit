#include "AudioManager.hpp"
#include <Levye/Assets/AssetPath.hpp>

#include <algorithm>
#include <iostream>
#include <utility>

namespace Levye {
AudioManager::~AudioManager() { Clear(); }

AssetHandle AudioManager::LoadSound(const std::string &path) {
  const std::string normalizedPath = AssetPath::Normalize(path);

  if (normalizedPath.empty())
    return {};
  const auto existing = m_PathLookup.find(normalizedPath);

  if (existing != m_PathLookup.end())
    return existing->second;

  Sound sound = ::LoadSound(normalizedPath.c_str());

  if (!IsSoundValid(sound)) {
    std::cerr << "[LevyeKit] Failed to load sound: " << path << '\n';

    return {};
  }

  const AssetHandle handle{.id = m_NextHandle++, .type = AssetType::Sound};

  SoundAsset asset{.sound = sound,
                   .path = normalizedPath,
                   .watcher = FileWatcher{},
                   .volume = 1.0f};

  /*
   * Watch the source file so the sound can be replaced at runtime while its
   * AssetHandle remains unchanged.
   */
  asset.watcher.Watch(normalizedPath);

  m_Sounds.emplace(handle.id, std::move(asset));

  m_PathLookup.emplace(normalizedPath, handle);

  std::cout << "[LevyeKit] Loaded sound: " << path << '\n';

  return handle;
}

void AudioManager::PlaySound(AssetHandle handle) {
  if (!handle.IsType(AssetType::Sound)) {
    return;
  }
  const auto iterator = m_Sounds.find(handle.id);

  if (iterator == m_Sounds.end())
    return;

  ::PlaySound(iterator->second.sound);
}

void AudioManager::StopSound(AssetHandle handle) {
  if (!handle.IsType(AssetType::Sound)) {
    return;
  }
  const auto iterator = m_Sounds.find(handle.id);

  if (iterator == m_Sounds.end())
    return;

  ::StopSound(iterator->second.sound);
}

void AudioManager::SetSoundVolume(AssetHandle handle, float volume) {
  if (!handle.IsType(AssetType::Sound)) {
    return;
  }

  const auto iterator = m_Sounds.find(handle.id);

  if (iterator == m_Sounds.end())
    return;

  SoundAsset &asset = iterator->second;

  asset.volume = volume;

  ::SetSoundVolume(asset.sound, volume);
}

void AudioManager::UnloadSound(AssetHandle handle) {
  if (!handle.IsType(AssetType::Sound)) {
    return;
  }
  const auto iterator = m_Sounds.find(handle.id);

  if (iterator == m_Sounds.end())
    return;

  ::UnloadSound(iterator->second.sound);

  m_PathLookup.erase(iterator->second.path);

  m_Sounds.erase(iterator);
}

AssetHandle AudioManager::LoadMusic(const std::string &path) {
  const std::string normalizedPath = AssetPath::Normalize(path);

  if (normalizedPath.empty())
    return {};
  const auto existing = m_MusicPathLookup.find(normalizedPath);

  if (existing != m_MusicPathLookup.end())
    return existing->second;

  Music music = ::LoadMusicStream(normalizedPath.c_str());

  if (!IsMusicValid(music)) {
    std::cerr << "[LevyeKit] Failed to load music: " << path << '\n';

    return {};
  }

  const AssetHandle handle{.id = m_NextHandle++, .type = AssetType::Music};

  MusicAsset asset{.music = music,
                   .path = normalizedPath,
                   .watcher = FileWatcher{},
                   .volume = 1.0f,
                   .playing = false,
                   .paused = false};

  /*
   * Music uses the same stable-file watcher as the other hot-reloadable asset
   * systems. Playback state is restored if the source changes.
   */
  asset.watcher.Watch(normalizedPath);

  m_Music.emplace(handle.id, std::move(asset));

  m_MusicPathLookup.emplace(normalizedPath, handle);

  std::cout << "[LevyeKit] Loaded music: " << path << '\n';

  return handle;
}

void AudioManager::PlayMusic(AssetHandle handle) {
  if (!handle.IsType(AssetType::Music)) {
    return;
  }
  const auto iterator = m_Music.find(handle.id);

  if (iterator == m_Music.end())
    return;

  MusicAsset &asset = iterator->second;

  ::PlayMusicStream(asset.music);
  asset.playing = true;
  asset.paused = false;
}

void AudioManager::PauseMusic(AssetHandle handle) {
  if (!handle.IsType(AssetType::Music)) {
    return;
  }
  const auto iterator = m_Music.find(handle.id);

  if (iterator == m_Music.end())
    return;

  MusicAsset &asset = iterator->second;

  if (!asset.playing)
    return;

  ::PauseMusicStream(asset.music);

  asset.paused = true;
}

void AudioManager::ResumeMusic(AssetHandle handle) {
  if (!handle.IsType(AssetType::Music)) {
    return;
  }
  const auto iterator = m_Music.find(handle.id);

  if (iterator == m_Music.end())
    return;

  MusicAsset &asset = iterator->second;

  ::ResumeMusicStream(asset.music);

  asset.playing = true;
  asset.paused = false;
}

void AudioManager::StopMusic(AssetHandle handle) {
  if (!handle.IsType(AssetType::Music)) {
    return;
  }
  const auto iterator = m_Music.find(handle.id);

  if (iterator == m_Music.end())
    return;

  MusicAsset &asset = iterator->second;

  ::StopMusicStream(asset.music);

  asset.playing = false;
  asset.paused = false;
}

void AudioManager::SetMusicVolume(AssetHandle handle, float volume) {
  if (!handle.IsType(AssetType::Music)) {
    return;
  }
  const auto iterator = m_Music.find(handle.id);

  if (iterator == m_Music.end())
    return;

  MusicAsset &asset = iterator->second;

  asset.volume = volume;

  ::SetMusicVolume(asset.music, volume);
}

void AudioManager::Update() {
  for (auto &[id, asset] : m_Music) {
    (void)id;

    if (::IsMusicStreamPlaying(asset.music)) {
      ::UpdateMusicStream(asset.music);
    }
  }
}

bool AudioManager::ReloadSound(SoundAsset &asset) {
  /*
   * Load the replacement before destroying the known-good sound. If loading
   * fails, the existing resource remains valid and its handle is unchanged.
   */
  Sound replacement = ::LoadSound(asset.path.c_str());

  if (!IsSoundValid(replacement)) {
    return false;
  }

  ::SetSoundVolume(replacement, asset.volume);

  ::UnloadSound(asset.sound);

  asset.sound = replacement;

  return true;
}

bool AudioManager::ReloadMusic(MusicAsset &asset) {
  const float playbackPosition = ::GetMusicTimePlayed(asset.music);

  Music replacement = ::LoadMusicStream(asset.path.c_str());

  if (!IsMusicValid(replacement)) {
    return false;
  }

  ::SetMusicVolume(replacement, asset.volume);

  if (asset.playing) {
    ::PlayMusicStream(replacement);

    const float length = ::GetMusicTimeLength(replacement);

    if (playbackPosition > 0.0f && playbackPosition < length) {
      ::SeekMusicStream(replacement, playbackPosition);
    }

    if (asset.paused) {
      ::PauseMusicStream(replacement);
    }
  }
  ::UnloadMusicStream(asset.music);

  asset.music = replacement;

  return true;
}

void AudioManager::CheckForChanges() {
  for (auto &[id, asset] : m_Sounds) {
    (void)id;

    if (!asset.watcher.Poll())
      continue;

    if (ReloadSound(asset)) {
      std::cout << "[LevyeKit] Reloaded sound: " << asset.path << '\n';
    } else {
      std::cerr << "[LevyeKit] Failed to reload sound: " << asset.path << '\n';
    }
  }

  for (auto &[id, asset] : m_Music) {
    (void)id;

    if (!asset.watcher.Poll())
      continue;

    if (ReloadMusic(asset)) {
      std::cout << "[LevyeKit] Reloaded music: " << asset.path << '\n';
    } else {
      std::cerr << "[LevyeKit] Failed to reload music: " << asset.path << '\n';
    }
  }
}

void AudioManager::UnloadMusic(AssetHandle handle) {
  if (!handle.IsType(AssetType::Music)) {
    return;
  }
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