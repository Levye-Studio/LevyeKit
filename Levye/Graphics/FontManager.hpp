#pragma once

#include <Levye/Assets/AssetHandle.hpp>
#include <Levye/IO/FileWatcher.hpp>

#include <raylib.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace Levye {
/**
 * @brief Owns font resources used by the game.
 *
 * Fonts remain owned by the host so they survive reloads of the game
 * dynamic library. Game code references fonts through AssetHandle values
 * rather than storing raw Font objects.
 *
 * Loaded font files are watched for changes and can be reloaded while the
 * application is running without changing their AssetHandle.
 */
class FontManager {
public:
  FontManager() = default;

  /**
   * @brief Releases all fonts still owned by the manager.
   */
  ~FontManager();

  FontManager(const FontManager &) = delete;
  FontManager &operator=(const FontManager &) = delete;

  /**
   * @brief Loads a font from disk.
   *
   * Loading the same path more than once returns the existing handle.
   *
   * @param path Path to the font file.
   * @param fontSize Base size used when rasterizing the font.
   * @return Handle to the loaded font, or an invalid handle on failure.
   */
  AssetHandle Load(const std::string &path, int fontSize);

  /**
   * @brief Returns the raylib font referenced by a handle.
   *
   * @param handle Font asset handle.
   * @return Pointer to the font, or nullptr if the handle is invalid.
   *
   * @warning The returned pointer must not be stored persistently.
   * Font hot reload may replace the underlying Font object.
   */
  const Font *Get(AssetHandle handle) const;

  /**
   * @brief Checks loaded font files for external changes.
   *
   * Changed fonts are reloaded while preserving their existing handles.
   *
   * @return Number of fonts successfully reloaded.
   */
  std::size_t CheckForChanges();

  /**
   * @brief Unloads one font.
   *
   * @param handle Font asset handle.
   */
  void Unload(AssetHandle handle);

  /**
   * @brief Releases every font owned by the manager.
   */
  void Clear();

private:
  /**
   * @brief Creates the lookup key used to identify a loaded font configuration.
   */
  static std::string CreateKey(const std::string &path, int fontSize);

private:
  struct FontAsset {
    Font font{};

    std::string path;
    int fontSize = 32;

    FileWatcher watcher;
  };

  /**
   * @brief Reloads a font while preserving its AssetHandle.
   *
   * The replacement is loaded before the existing font is destroyed so
   * a failed reload leaves the known-good resource available.
   *
   * @param asset Font asset to reload.
   * @return true if the replacement was installed successfully.
   */
  bool Reload(FontAsset &asset);

  std::unordered_map<std::uint64_t, FontAsset> m_Fonts;

  std::unordered_map<std::string, AssetHandle> m_PathLookup;

  std::uint64_t m_NextHandle = 1;
};
} // namespace Levye