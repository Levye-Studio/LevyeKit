#include "ShaderManager.hpp"

#include <iostream>

namespace Levye {
namespace {
std::filesystem::file_time_type GetWriteTime(const std::string &path) {
  if (path.empty())
    return {};

  std::error_code error;

  const auto time = std::filesystem::last_write_time(path, error);

  if (error)
    return {};

  return time;
}
} // namespace

ShaderManager::~ShaderManager() { Clear(); }

AssetHandle ShaderManager::Load(const std::string &vertexPath,
                                const std::string &fragmentPath) {
  const char *vertex = vertexPath.empty() ? nullptr : vertexPath.c_str();

  const char *fragment = fragmentPath.empty() ? nullptr : fragmentPath.c_str();

  Shader shader = ::LoadShader(vertex, fragment);

  if (!IsShaderValid(shader)) {
    std::cerr << "[LevyeKit] Failed to load shader.\n";

    return {};
  }

  const AssetHandle handle{.id = m_NextHandle++};

  m_Shaders.emplace(
      handle.id,
      ShaderAsset{.shader = shader,

                  .vertexPath = vertexPath,
                  .fragmentPath = fragmentPath,

                  .vertexWatcher = vertexPath.empty() ? FileWatcher{}
                                                      : FileWatcher{vertexPath},

                  .fragmentWatcher = fragmentPath.empty()
                                         ? FileWatcher{}
                                         : FileWatcher{fragmentPath}});

  std::cout << "[LevyeKit] Loaded shader: " << vertexPath << " | "
            << fragmentPath << '\n';

  return handle;
}

const Shader *ShaderManager::Get(AssetHandle handle) const {
  const auto iterator = m_Shaders.find(handle.id);

  if (iterator == m_Shaders.end())
    return nullptr;

  return &iterator->second.shader;
}
bool ShaderManager::Reload(ShaderAsset &asset) {
  const char *vertex =
      asset.vertexPath.empty() ? nullptr : asset.vertexPath.c_str();

  const char *fragment =
      asset.fragmentPath.empty() ? nullptr : asset.fragmentPath.c_str();

  /*
   * Build the replacement before destroying the currently active program.
   * A failed reload must not invalidate the known-good shader.
   */
  Shader replacement = ::LoadShader(vertex, fragment);

  if (!IsShaderValid(replacement)) {
    std::cerr << "[LevyeKit] Failed to reload shader.\n";

    return false;
  }

  ::UnloadShader(asset.shader);

  asset.shader = replacement;

  std::cout << "[LevyeKit] Reloaded shader: " << asset.vertexPath << " | "
            << asset.fragmentPath << '\n';

  return true;
}

std::size_t ShaderManager::CheckForChanges() {
  std::size_t reloadCount = 0;

  for (auto &[id, asset] : m_Shaders) {
    (void)id;

    const bool vertexChanged = asset.vertexWatcher.Poll();

    const bool fragmentChanged = asset.fragmentWatcher.Poll();

    if (!vertexChanged && !fragmentChanged) {
      continue;
    }

    if (Reload(asset))
      ++reloadCount;
  }

  return reloadCount;
}

void ShaderManager::Unload(AssetHandle handle) {
  const auto iterator = m_Shaders.find(handle.id);

  if (iterator == m_Shaders.end())
    return;

  ::UnloadShader(iterator->second.shader);

  m_Shaders.erase(iterator);
}

void ShaderManager::Clear() {
  for (auto &[id, asset] : m_Shaders) {
    (void)id;

    ::UnloadShader(asset.shader);
  }

  m_Shaders.clear();
}
} // namespace Levye