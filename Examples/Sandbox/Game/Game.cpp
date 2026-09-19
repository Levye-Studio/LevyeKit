#include <Levye/Core/GameAPI.hpp>

#include <raylib.h>

namespace {
void OnLoad(Levye::GameState *state) { state->initialized = true; }

void OnReload(Levye::GameState *state) {
  /*
   * Persistent state already exists here. Do not perform normal
   * first-launch initialization during a hot reload.
   */
  state->initialized = true;
}

void OnUpdate(Levye::GameState *state, float deltaTime) {
  (void)state;
  (void)deltaTime;
}

void OnDraw(Levye::GameState *state) {
  if (!state->initialized)
    return;

  ClearBackground(MAROON);

  DrawText("LevyeKit HOT RELOAD", 40, 40, 32, RAYWHITE);

  DrawText(TextFormat("Reloads: %i", state->reloadCount), 40, 90, 24,
           LIGHTGRAY);
}

void OnUnload(Levye::GameState *state) {
  /*
   * Do not erase persistent gameplay state here.
   *
   * OnUnload is also called immediately before hot reload, so clearing
   * persistent data here would defeat state preservation.
   */
  (void)state;
}
} // namespace

/**
 * @brief Returns the public API implemented by the Sandbox game module.
 *
 * This is the single exported entry point required by LevyeKit.
 *
 * extern "C" disables C++ name mangling so the host can reliably locate
 * this function using the symbol name "GetGameAPI".
 */
extern "C" Levye::GameAPI GetGameAPI() {
  return {.OnLoad = OnLoad,
          .OnReload = OnReload,
          .OnUpdate = OnUpdate,
          .OnDraw = OnDraw,
          .OnUnload = OnUnload};
}