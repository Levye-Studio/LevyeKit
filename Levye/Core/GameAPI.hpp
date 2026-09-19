#pragma once
#include "HostServices.hpp"
#include <Levye/Assets/AssetHandle.hpp>
#include <raylib.h>

namespace Levye {
/**
 * @brief Persistent state owned by the Levye host application.
 */
struct GameState {
  bool initialized = false;
  int reloadCount = 0;

  Vector2 playerPosition{640.0f, 360.0f};

  AssetHandle playerTexture{};
};

/**
 * @brief Function table exposed by every Levye game module.
 */
struct GameAPI {
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