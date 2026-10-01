#include "FontManager.hpp"
#include <Levye/Debug/Logger.hpp>
#include <Levye/Assets/AssetPath.hpp>


namespace Levye {
FontManager::~FontManager() { Clear(); }

AssetHandle FontManager::Load(const std::string &path, int fontSize) {
  if (path.empty() || fontSize <= 0) {
    return {};
  }

  const std::string normalizedPath = AssetPath::Normalize(path);

  if (normalizedPath.empty() || fontSize <= 0) {
    return {};
  }

  const std::string key = CreateKey(normalizedPath, fontSize);

  /*
   * Return the existing host-owned resource instead of loading the same
   * font file multiple times.
   */
  const auto existing = m_PathLookup.find(key);

  if (existing != m_PathLookup.end()) {
    return existing->second;
  }

  Font font = ::LoadFontEx(normalizedPath.c_str(), fontSize, nullptr, 0);

  if (!::IsFontValid(font)) {
    Logger::Error("Failed to load font: " + path);

    return {};
  }

  AssetHandle handle{.id = m_NextHandle++, .type = AssetType::Font};

  m_Fonts.emplace(handle.id, FontAsset{.font = font,
                                       .path = normalizedPath,
                                       .fontSize = fontSize,
                                       .watcher = FileWatcher(normalizedPath)});

  m_PathLookup.emplace(key, handle);

  Logger::Info("Loaded font: " + path);

  return handle;
}

const Font *FontManager::Get(AssetHandle handle) const {
  if (!handle.IsType(AssetType::Font)) {
    return nullptr;
  }

  const auto it = m_Fonts.find(handle.id);

  if (it == m_Fonts.end())
    return nullptr;

  return &it->second.font;
}

std::size_t FontManager::CheckForChanges() {
  std::size_t reloadCount = 0;

  for (auto &[id, asset] : m_Fonts) {
    (void)id;

    if (!asset.watcher.Poll())
      continue;

    if (Reload(asset))
      ++reloadCount;
  }

  return reloadCount;
}

void FontManager::Unload(AssetHandle handle) {
  if (!handle.IsType(AssetType::Font)) {
    return;
  }

  const auto it = m_Fonts.find(handle.id);

  if (it == m_Fonts.end())
    return;

  m_PathLookup.erase(CreateKey(it->second.path, it->second.fontSize));

  ::UnloadFont(it->second.font);

  m_Fonts.erase(it);
}

void FontManager::Clear() {
  for (auto &[id, asset] : m_Fonts) {
    (void)id;

    ::UnloadFont(asset.font);
  }

  m_Fonts.clear();
  m_PathLookup.clear();
}

bool FontManager::Reload(FontAsset &asset) {
  /*
   * Load the replacement first. Never destroy the currently working
   * font until the new resource has been validated.
   */
  Font replacement =
      ::LoadFontEx(asset.path.c_str(), asset.fontSize, nullptr, 0);

  if (!::IsFontValid(replacement)) {
    Logger::Error("Failed to reload font: " + asset.path);

    return false;
  }

  ::UnloadFont(asset.font);

  asset.font = replacement;

  Logger::Info("Reloaded font: " + asset.path);

  return true;
}

std::string FontManager::CreateKey(const std::string &path, int fontSize) {
  return path + "#" + std::to_string(fontSize);
}
} // namespace Levye