#include <Levye/Core/GameAPI.hpp>

#include <raylib.h>
#include <raymath.h>
namespace {
void OnLoad(Levye::GameState *state, const Levye::HostServices *services) {
  state->initialized = true;

  state->playerTexture = services->LoadTexture(
      services->context, "Examples/Sandbox/Assets/player.png");

  state->clickSound = services->LoadSound(services->context,
                                          "Examples/Sandbox/Assets/click.wav");

  state->music = services->LoadMusic(services->context,
                                     "Examples/Sandbox/Assets/music.ogg");

  services->SetMusicVolume(services->context, state->music, 0.5f);

  services->PlayMusic(services->context, state->music);

  state->playerShader = services->LoadShader(services->context, nullptr,
                                             "Examples/Sandbox/Assets/test.fs");

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

    constexpr float speed = 100.0f;

    Vector2 movement{services->GetAxis(services->context, "MoveX"),

                     services->GetAxis(services->context, "MoveY")};

    if (Vector2Length(movement) > 1.0f) {
      movement = Vector2Normalize(movement);
    }

    state->playerPosition.x += movement.x * speed * deltaTime;

    state->playerPosition.y += movement.y * speed * deltaTime;

    if (services->IsActionPressed(services->context, "TestSound")) {
      services->PlaySound(services->context, state->clickSound);
    }

    if (services->IsActionPressed(services->context, "PauseMusic")) {
      services->PauseMusic(services->context, state->music);
    }

    if (services->IsActionPressed(services->context, "ResumeMusic")) {
      services->ResumeMusic(services->context, state->music);
    }

    if (services->IsActionPressed(services->context, "NormalTime")) {
      services->SetTimeScale(services->context, 1.0f);
    }

    if (services->IsActionPressed(services->context, "SlowTime")) {
      services->SetTimeScale(services->context, 0.25f);
    }

    if (services->IsActionPressed(services->context, "FastTime")) {
      services->SetTimeScale(services->context, 2.0f);
    }

    if (services->IsActionPressed(services->context, "Pause")) {
      const bool paused = services->IsPaused(services->context);

      services->SetPaused(services->context, !paused);
    }
  }
}

void OnDraw(Levye::GameState *state, const Levye::HostServices *services) {
  if (!state->initialized)
    return;

  const Texture2D *playerTexture =
      services->GetTexture(services->context, state->playerTexture);

  const Shader *playerShader =
      services->GetShader(services->context, state->playerShader);

  if (services->IsScreen(services->context, "Menu")) {
    ClearBackground(BLACK);

    DrawText("LEVYEKIT", 40, 40, 40, RAYWHITE);

    DrawText("Press ENTER to play", 40, 100, 24, LIGHTGRAY);

    return;
  }

  if (services->IsScreen(services->context, "Game")) {
    ClearBackground(DARKBLUE);

    if (playerTexture && playerShader) {
      BeginShaderMode(*playerShader);

      DrawTexture(*playerTexture, static_cast<int>(state->playerPosition.x),
                  static_cast<int>(state->playerPosition.y), WHITE);

      EndShaderMode();
    } else if (playerTexture) {
      DrawTexture(*playerTexture, static_cast<int>(state->playerPosition.x),
                  static_cast<int>(state->playerPosition.y), RED);
    } else {

      DrawCircleV(state->playerPosition, 30.0f, GOLD);
    }

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
  return {.version = Levye::GAME_API_VERSION,

          .OnLoad = OnLoad,
          .OnReload = OnReload,
          .OnUpdate = OnUpdate,
          .OnDraw = OnDraw,
          .OnUnload = OnUnload};
}