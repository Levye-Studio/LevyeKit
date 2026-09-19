#include "TextureManager.hpp"

#include <iostream>

namespace Levye {
TextureManager::~TextureManager() { Clear(); }

AssetHandle TextureManager::Load(const std::string &path) {
  /*
   * Reuse an existing texture when the same path has already been
   * loaded. This avoids duplicate GPU resources.
   */
  const auto existing = m_PathLookup.find(path);
  if (existing != m_PathLookup.end())
    return existing->second;

  Texture2D texture = LoadTexture(path.c_str());

  if (!IsTextureValid(texture)) {
    std::cerr << "[LevyeKit] Failed to load texture: " << path << '\n';

    return {};
  }
  std::error_code timeError;
  std::error_code sizeError;

  const auto lastWriteTime = std::filesystem::last_write_time(path, timeError);

  const auto fileSize = std::filesystem::file_size(path, sizeError);

  const AssetHandle handle{.id = m_NextHandle++};

  m_Textures.emplace(
      handle.id, TextureAsset{.texture = texture,
                              .path = path,

                              .lastWriteTime =
                                  timeError ? std::filesystem::file_time_type{}
                                            : lastWriteTime,

                              .fileSize = sizeError ? 0 : fileSize});

  m_PathLookup.emplace(path, handle);

  std::cout << "[LevyeKit] Loaded texture: " << path << '\n';

  return handle;
}

const Texture2D *TextureManager::Get(AssetHandle handle) const {
  if (!handle.IsValid())
    return nullptr;

  const auto iterator = m_Textures.find(handle.id);

  if (iterator == m_Textures.end())
    return nullptr;

  return &iterator->second.texture;
}

void TextureManager::Unload(AssetHandle handle) {
  const auto iterator = m_Textures.find(handle.id);

  if (iterator == m_Textures.end())
    return;

  UnloadTexture(iterator->second.texture);

  m_PathLookup.erase(iterator->second.path);

  m_Textures.erase(iterator);
}

bool TextureManager::Reload(TextureAsset &asset) {
  /*
   * Load the replacement first. The currently active GPU texture remains
   * untouched until we know the new resource is valid.
   */
  Texture2D replacement = LoadTexture(asset.path.c_str());

  if (!IsTextureValid(replacement)) {
    std::cerr << "[LevyeKit] Failed to reload texture: " << asset.path << '\n';

    return false;
  }

  /*
   * Only destroy the old GPU resource after the replacement has been
   * created successfully.
   */
  UnloadTexture(asset.texture);

  asset.texture = replacement;

  std::cout << "[LevyeKit] Reloaded texture: " << asset.path << '\n';

  return true;
}

std::size_t TextureManager::CheckForChanges() {
  std::size_t reloadCount = 0;

  for (auto &[id, asset] : m_Textures) {

    std::error_code timeError;
    std::error_code sizeError;

    const auto currentWriteTime =
        std::filesystem::last_write_time(asset.path, timeError);

    const auto currentFileSize =
        std::filesystem::file_size(asset.path, sizeError);

    if (timeError || sizeError)
      continue;

    const bool timeChanged = currentWriteTime != asset.lastWriteTime;

    const bool sizeChanged = currentFileSize != asset.fileSize;

    if (!timeChanged && !sizeChanged)
      continue;

    std::cout << "[LevyeKit] Texture change detected: " << asset.path
              << " | handle=" << id << '\n';

    /*
     * Remember the version we observed. A failed asset should not trigger
     * another reload every frame until the source file changes again.
     */
    asset.lastWriteTime = currentWriteTime;

    asset.fileSize = currentFileSize;

    if (Reload(asset))
      ++reloadCount;
  }

  return reloadCount;
}

void TextureManager::Clear() {
  for (auto &[id, asset] : m_Textures) {
    (void)id;

    UnloadTexture(asset.texture);
  }

  m_Textures.clear();
  m_PathLookup.clear();
}
} // namespace Levye