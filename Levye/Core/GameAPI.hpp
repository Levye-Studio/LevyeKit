#pragma once
#include "HostServices.hpp"
#include <Levye/Assets/AssetHandle.hpp>
#include <raylib.h>

#include <cstdint>

namespace Levye {
/**
 * @brief Version of the binary interface shared by the host and game module.
 */
inline constexpr std::uint32_t GAME_API_VERSION = 3;

/**
 * @brief Persistent state owned by the Levye host application.
 */
struct GameState {
  bool initialized = false;
  int reloadCount = 0;

  Vector2 playerPosition{640.0f, 360.0f};

  AssetHandle playerTexture{};
  AssetHandle clickSound{};
  AssetHandle music{};
  AssetHandle playerShader{};
};

/**
 * @brief Function table exposed by every Levye game module.
 */
struct GameAPI {
  std::uint32_t version = GAME_API_VERSION;

  /// Called after the game module has been loaded.
  void (*OnLoad)(GameState *state, const HostServices *services) = nullptr;

  /// Called after game code has been successfully hot reloaded.
  void (*OnReload)(GameState *state, const HostServices *services) = nullptr;

  /// Called once per frame before rendering.
  void (*OnUpdate)(GameState *state, const HostServices *services,
                   float deltaTime) = nullptr;

  /// Called once per frame while a raylib drawing context is active.
  void (*OnDraw)(GameState *state, const HostServices *services) = nullptr;

  /// Called before the game module is unloaded.
  void (*OnUnload)(GameState *state, const HostServices *services) = nullptr;
};

/**
 * @brief Signature of the entry point exported by a Levye game module.
 */
using GetGameAPIFn = GameAPI (*)();
} // namespace Levye