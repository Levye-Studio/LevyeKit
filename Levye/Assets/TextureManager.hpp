#pragma once

#include <Levye/Assets/AssetHandle.hpp>

#include <raylib.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace Levye {
/**
 * @brief Owns texture resources used by the game.
 *
 * TextureManager lives in the host process so loaded GPU resources remain
 * valid when reloadable game code is replaced.
 *
 * Game code refers to textures using AssetHandle rather than owning
 * Texture2D objects directly.
 */
class TextureManager {
public:
  /**
   * @brief Releases every texture still owned by the manager.
   */
  ~TextureManager();

  TextureManager(const TextureManager &) = delete;
  TextureManager &operator=(const TextureManager &) = delete;

  TextureManager() = default;

  /**
   * @brief Loads a texture from disk.
   *
   * Loading the same path more than once returns the existing handle
   * instead of creating another GPU texture.
   *
   * @param path Path to the texture file.
   * @return Handle to the loaded texture, or an invalid handle on failure.
   */
  AssetHandle Load(const std::string &path);

  /**
   * @brief Returns a loaded texture.
   *
   * @param handle Handle returned by Load().
   * @return Pointer to the texture, or nullptr if the handle is invalid.
   *
   * @warning The returned pointer is owned by TextureManager and must not
   * be unloaded by game code.
   */
  const Texture2D *Get(AssetHandle handle) const;

  /**
   * @brief Checks loaded texture files for changes.
   *
   * Changed textures are reloaded while preserving their existing AssetHandle.
   *
   * @return Number of textures successfully reloaded.
   */
  std::size_t CheckForChanges();

  /**
   * @brief Unloads a texture and invalidates its handle.
   *
   * @param handle Texture handle to unload.
   */
  void Unload(AssetHandle handle);

  /**
   * @brief Unloads every texture owned by the manager.
   */
  void Clear();

private:
  struct TextureAsset {
    Texture2D texture{};
    std::string path;

    // Last known modification time of the source asset.
    std::filesystem::file_time_type lastWriteTime{};

    std::uintmax_t fileSize = 0;
  };

private:
  /**
   * @brief Attempts to replace an existing GPU texture from its source file.
   *
   * The old texture remains valid if loading the replacement fails.
   *
   * @param asset Texture asset to reload.
   * @return true when the replacement succeeded.
   */
  bool Reload(TextureAsset &asset);

private:
  std::unordered_map<std::uint64_t, TextureAsset> m_Textures;

  std::unordered_map<std::string, AssetHandle> m_PathLookup;

  std::uint64_t m_NextHandle = 1;
};
} // namespace Levye