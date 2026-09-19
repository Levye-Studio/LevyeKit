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

  const AssetHandle handle{.id = m_NextHandle++};

  m_Textures.emplace(handle.id, TextureAsset{.texture = texture, .path = path});

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

void TextureManager::Clear() {
  for (auto &[id, asset] : m_Textures) {
    (void)id;

    UnloadTexture(asset.texture);
  }

  m_Textures.clear();
  m_PathLookup.clear();
}
} // namespace Levye