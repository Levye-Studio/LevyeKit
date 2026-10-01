#pragma once

#include <Levye/Core/Services.hpp>

#include <raylib.h>

namespace Levye {

/**
 * @brief Provides access to host-owned game assets.
 *
 * Assets loads and retrieves resources through LevyeKit's host-owned asset
 * managers. AssetHandle values remain valid across compatible game-code
 * hot reloads because the underlying resources are owned by the host.
 */
class Assets {
public:
  // ---------------------------------------------------------------------
  // Textures
  // ---------------------------------------------------------------------

  /**
   * @brief Loads or retrieves a texture.
   *
   * Relative paths are resolved against the project's asset directory.
   *
   * @param path Path to the texture asset.
   * @return Handle representing the texture, or an invalid handle on
   * failure.
   */
  static AssetHandle LoadTexture(const char *path) {
    const HostServices *host = Services::Get();

    if (!host || !host->LoadTexture || !path) {
      return {};
    }

    return host->LoadTexture(host->context, path);
  }

  /**
   * @brief Retrieves a loaded texture.
   *
   * @param handle Texture asset handle.
   * @return Pointer to the host-owned raylib texture, or nullptr when the
   * handle is invalid.
   *
   * @warning The returned pointer is host-owned and must not be unloaded
   * or freed by game code.
   */
  static const Texture2D *GetTexture(AssetHandle handle) {
    const HostServices *host = Services::Get();

    if (!host || !host->GetTexture) {
      return nullptr;
    }

    return host->GetTexture(host->context, handle);
  }

  // ---------------------------------------------------------------------
  // Shaders
  // ---------------------------------------------------------------------

  /**
   * @brief Loads or retrieves a shader.
   *
   * Either shader path may be nullptr when only one shader stage is used.
   * Relative paths are resolved against the project's asset directory.
   *
   * @param vertexPath Path to the vertex shader, or nullptr.
   * @param fragmentPath Path to the fragment shader, or nullptr.
   * @return Handle representing the shader, or an invalid handle on
   * failure.
   */
  static AssetHandle LoadShader(const char *vertexPath,
                                const char *fragmentPath) {
    const HostServices *host = Services::Get();

    if (!host || !host->LoadShader) {
      return {};
    }

    return host->LoadShader(host->context, vertexPath, fragmentPath);
  }

  /**
   * @brief Retrieves a loaded shader.
   *
   * @param handle Shader asset handle.
   * @return Pointer to the host-owned raylib shader, or nullptr when the
   * handle is invalid.
   *
   * @warning The returned pointer is host-owned and must not be unloaded
   * by game code.
   */
  static const Shader *GetShader(AssetHandle handle) {
    const HostServices *host = Services::Get();

    if (!host || !host->GetShader) {
      return nullptr;
    }

    return host->GetShader(host->context, handle);
  }

  // ---------------------------------------------------------------------
  // Fonts
  // ---------------------------------------------------------------------

  /**
   * @brief Loads or retrieves a font.
   *
   * Relative paths are resolved against the project's asset directory.
   *
   * @param path Path to the font asset.
   * @param fontSize Font size used when loading the asset.
   * @return Handle representing the font, or an invalid handle on failure.
   */
  static AssetHandle LoadFont(const char *path, int fontSize) {
    const HostServices *host = Services::Get();

    if (!host || !host->LoadFont || !path || fontSize <= 0) {
      return {};
    }

    return host->LoadFont(host->context, path, fontSize);
  }

  /**
   * @brief Retrieves a loaded font.
   *
   * @param handle Font asset handle.
   * @return Pointer to the host-owned raylib font, or nullptr when the
   * handle is invalid.
   *
   * @warning The returned pointer is host-owned and must not be unloaded
   * by game code.
   */
  static const Font *GetFont(AssetHandle handle) {
    const HostServices *host = Services::Get();

    if (!host || !host->GetFont) {
      return nullptr;
    }

    return host->GetFont(host->context, handle);
  }
};

} // namespace Levye