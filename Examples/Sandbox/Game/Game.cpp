#include <Levye/Core/GameAPI.hpp>

#include <raylib.h>

namespace {
void OnLoad(Levye::GameState *state) { state->initialized = true; }

void OnUpdate(Levye::GameState *state, float deltaTime) {
  (void)state;
  (void)deltaTime;
}

void OnDraw(Levye::GameState *state) {
  if (!state->initialized)
    return;

  ClearBackground(DARKBLUE);

  DrawText("Hello from Game.dylib!", 40, 40, 32, RAYWHITE);
}

void OnUnload(Levye::GameState *state) { state->initialized = false; }
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
          .OnUpdate = OnUpdate,
          .OnDraw = OnDraw,
          .OnUnload = OnUnload};
}