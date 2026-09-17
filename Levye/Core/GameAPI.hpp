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
};

/**
 * @brief Function table used by LevyeKit to communicate with a game module.
 *
 * A game module exposes implementations of these callbacks. LevyeKit keeps
 * the application/window alive and invokes the callbacks during the game
 * lifecycle.
 *
 * Using a function table creates a stable boundary between the host
 * executable and reloadable game code.
 */
struct GameAPI {
  /// Called after the game module has been loaded.
  void (*OnLoad)(GameState *state);

  /// Called once per frame before rendering.
  void (*OnUpdate)(GameState *state, float deltaTime);

  /// Called once per frame while a raylib drawing context is active.
  void (*OnDraw)(GameState *state);

  /// Called before the game module is unloaded.
  void (*OnUnload)(GameState *state);
};

/**
 * @brief Signature of the function exported by every Levye game module.
 *
 * DynamicLibrary retrieves this function from the loaded shared library
 * and uses it to obtain the module's GameAPI.
 */
using GetGameAPIFn = GameAPI (*)();
} // namespace Levye