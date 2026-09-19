#include <Levye/Core/GameAPI.hpp>

#include <raylib.h>
#include <raymath.h>
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
  // (void)services;

  if (services->IsScreen(services->context, "Menu")) {
    if (services->IsActionPressed(services->context, "Confirm")) {
      services->SetScreen(services->context, "Game");
    }

    return;
  }

  if (services->IsScreen(services->context, "Game")) {

    if (services->IsActionPressed(services->context, "Back")) {
      services->SetScreen(services->context, "Menu");

      return;
    }

    constexpr float speed = 800.0f;

    Vector2 movement{services->GetAxis(services->context, "MoveX"),

                     services->GetAxis(services->context, "MoveY")};

    if (Vector2Length(movement) > 1.0f) {
      movement = Vector2Normalize(movement);
    }

    state->playerPosition.x += movement.x * speed * deltaTime;

    state->playerPosition.y += movement.y * speed * deltaTime;
  }
}

void OnDraw(Levye::GameState *state, const Levye::HostServices *services) {
  if (!state->initialized)
    return;

  if (services->IsScreen(services->context, "Menu")) {
    ClearBackground(BLACK);

    DrawText("LEVYEKIT", 40, 40, 40, RAYWHITE);

    DrawText("Press ENTER to play", 40, 100, 24, LIGHTGRAY);

    return;
  }

  if (services->IsScreen(services->context, "Game")) {
    ClearBackground(DARKBLUE);

    DrawCircleV(state->playerPosition, 30.0f, GOLD);

    DrawText("Move: WASD / Arrows / Controller", 40, 40, 20, RAYWHITE);

    DrawText("ESC: Menu", 40, 70, 20, LIGHTGRAY);

    DrawText(TextFormat("Reloads: %i", state->reloadCount), 40, 100, 20,
             LIGHTGRAY);
  }
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