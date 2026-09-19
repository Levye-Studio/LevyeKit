#pragma once

namespace Levye {
/**
 * @brief Persistent state owned by the Levye host application.
 *
 * GameState is intentionally stored outside the reloadable game module.
 * This allows game code to be unloaded and replaced while keeping state
 * alive between hot reloads.
 *
 * This structure is minimal for now and will evolve as LevyeKit's
 * hot-reloading system develops.
 */
struct GameState {
  bool initialized = false;
  int reloadCount = 0;
};

/**
 * @brief Function table exposed by every Levye game module.
 *
 * The host uses this table as the stable interface between LevyeKit and
 * dynamically loaded game code.
 */
struct GameAPI {
  /// Called after the game module has been loaded.
  void (*OnLoad)(GameState *state);

  /// Called after game code has been successfully hot reloaded.
  void (*OnReload)(GameState *state);

  /// Called once per frame before rendering.
  void (*OnUpdate)(GameState *state, float deltaTime);

  /// Called once per frame while a raylib drawing context is active.
  void (*OnDraw)(GameState *state);

  /// Called before the game module is unloaded.
  void (*OnUnload)(GameState *state);
};

/**
 * @brief Signature of the entry point exported by a Levye game module.
 */
using GetGameAPIFn = GameAPI (*)();
} // namespace Levye