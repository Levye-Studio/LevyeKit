#pragma once

#include <Levye/Assets/AssetHandle.hpp>
#include <Levye/IO/FileWatcher.hpp>

#include <raylib.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace Levye {
/**
 * @brief Owns shader resources used by the game.
 *
 * Shaders live in the host so GPU shader programs remain valid when the
 * reloadable game module is replaced.
 *
 * Game code refers to shaders through AssetHandle.
 */
class ShaderManager {
public:
  ShaderManager() = default;

  /**
   * @brief Releases all shaders still owned by the manager.
   */
  ~ShaderManager();

  ShaderManager(const ShaderManager &) = delete;
  ShaderManager &operator=(const ShaderManager &) = delete;

  /**
   * @brief Loads a shader from source files.
   *
   * Either path may be empty to request raylib's default shader stage.
   *
   * @param vertexPath Path to the vertex shader, or empty for default.
   * @param fragmentPath Path to the fragment shader, or empty for default.
   * @return Handle to the shader, or an invalid handle on failure.
   */
  AssetHandle Load(const std::string &vertexPath,
                   const std::string &fragmentPath);

  /**
   * @brief Retrieves a host-owned shader.
   *
   * @param handle Shader handle.
   * @return Pointer to the shader, or nullptr when the handle is invalid.
   *
   * @warning The returned pointer must not be stored persistently or
   * unloaded by game code.
   */
  const Shader *Get(AssetHandle handle) const;

  /**
   * @brief Checks shader source files and reloads changed programs.
   *
   * Existing AssetHandle values remain valid after successful reloads.
   *
   * @return Number of shaders successfully reloaded.
   */
  std::size_t CheckForChanges();

  /**
   * @brief Unloads one shader.
   */
  void Unload(AssetHandle handle);

  /**
   * @brief Releases every shader owned by the manager.
   */
  void Clear();

private:
  struct ShaderAsset {
    Shader shader{};

    std::string vertexPath;
    std::string fragmentPath;

    FileWatcher vertexWatcher;
    FileWatcher fragmentWatcher;
  };

  /**
   * @brief Attempts to replace an existing shader program.
   *
   * The previous shader remains active if the replacement cannot be
   * created.
   */
  bool Reload(ShaderAsset &asset);

  std::unordered_map<std::uint64_t, ShaderAsset> m_Shaders;

  std::uint64_t m_NextHandle = 1;
};
} // namespace Levye