#include <Levye/Core/GameAPI.hpp>

#include <raylib.h>

namespace {
void OnLoad(Levye::GameState *state, const Levye::HostServices *services) {
  state->initialized = true;

  if (services->LogInfo)
    services->LogInfo("Sandbox started");
}

void OnReload(Levye::GameState *state, const Levye::HostServices *services) {
  /*
   * Persistent state already exists here. Do not perform normal
   * first-launch initialization during a hot reload.
   */
  state->initialized = true;

  if (services->LogInfo)
    services->LogInfo("Sandbox code reloaded.");
}

void OnUpdate(Levye::GameState *state, const Levye::HostServices *services,
              float deltaTime) {
  //   (void)state;
  //   (void)deltaTime;
  (void)services;

  constexpr float speed = 300.0f;

  if (services->IsActionDown(services->context, "MoveUp"))
    state->playerPosition.y -= speed * deltaTime;

  if (services->IsActionDown(services->context, "MoveDown"))
    state->playerPosition.y += speed * deltaTime;

  if (services->IsActionDown(services->context, "MoveLeft"))
    state->playerPosition.x -= speed * deltaTime;

  if (services->IsActionDown(services->context, "MoveRight"))
    state->playerPosition.x += speed * deltaTime;
}

void OnDraw(Levye::GameState *state, const Levye::HostServices *services) {
  if (!state->initialized)
    return;

  (void)services;

  ClearBackground(DARKBLUE);

  DrawCircleV(state->playerPosition, 30.0f, GOLD);

  DrawText("WASD to move", 40, 40, 24, RAYWHITE);

  DrawText(TextFormat("Reloads: %i", state->reloadCount), 40, 75, 20,
           LIGHTGRAY);
}

void OnUnload(Levye::GameState *state, const Levye::HostServices *services) {
  /*
   * Do not erase persistent gameplay state here.
   *
   * OnUnload is also called immediately before hot reload, so clearing
   * persistent data here would defeat state preservation.
   */
  (void)state;

  if (services->LogInfo)
    services->LogInfo("Sandbix module unloading.");
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