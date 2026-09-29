#include <Levye/Core/GameAPI.hpp>

#include <raylib.h>
#include <raymath.h>
namespace {
void OnLoad(Levye::GameState *state, const Levye::HostServices *services) {
  state->initialized = true;

  state->playerTexture = services->LoadTexture(services->context, "player.png");

  state->clickSound = services->LoadSound(services->context, "click.wav");

  state->music = services->LoadMusic(services->context, "music.ogg");

  services->SetMusicVolume(services->context, state->music, 0.5f);

  services->PlayMusic(services->context, state->music);

  state->playerShader =
      services->LoadShader(services->context, nullptr, "test.fs");

  state->font = services->LoadFont(services->context, "test.ttf", 32);

  if (services->LogInfo)
    services->LogInfo("Sandbox started");
}

void OnAfterReload(Levye::GameState *state,
                   const Levye::HostServices *services) {
  (void)state;
  if (services->LogInfo)
    services->LogInfo("Sandbox code reloaded.");
}

void OnBeforeReload(Levye::GameState *state,
                    const Levye::HostServices *services) {
  /*
   * This callback runs while the old game module is still loaded.
   *
   * Use it for temporary module-specific cleanup if needed, but do not
   * destroy persistent gameplay state or host-owned resources.
   */
  (void)state;

  if (services->LogInfo)
    services->LogInfo("Sandbox preparing for code reload.");
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

    // constexpr float speed = 100.0f;

    state->movementInput = {services->GetAxis(services->context, "MoveX"),

                            services->GetAxis(services->context, "MoveY")};

    if (Vector2Length(state->movementInput) > 1.0f) {
      state->movementInput = Vector2Normalize(state->movementInput);
    }

    // Vector2 movement{services->GetAxis(services->context, "MoveX"),

    //                  services->GetAxis(services->context, "MoveY")};

    // if (Vector2Length(movement) > 1.0f) {
    //   movement = Vector2Normalize(movement);
    // }

    // state->playerPosition.x += movement.x * speed * deltaTime;

    // state->playerPosition.y += movement.y * speed * deltaTime;

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

void OnFixedUpdate(Levye::GameState *state, const Levye::HostServices *services,
                   float fixedDeltaTime) {
  (void)services;
  constexpr float playerSpeed = 300.0f;

  /*
   * Preserve the previous simulation position before advancing the
   * current position. Rendering interpolates between these two states.
   */
  state->playerPreviousPosition = state->playerPosition;

  /*
   * Apply the input captured during OnUpdate to the fixed-rate simulation.
   */
  state->playerPosition.x +=
      state->movementInput.x * playerSpeed * fixedDeltaTime;

  state->playerPosition.y +=
      state->movementInput.y * playerSpeed * fixedDeltaTime;

  ++state->fixedUpdateCount;
}

void OnDraw(Levye::GameState *state, const Levye::HostServices *services) {
  if (!state->initialized)
    return;

  const Texture2D *playerTexture =
      services->GetTexture(services->context, state->playerTexture);

  const Shader *playerShader =
      services->GetShader(services->context, state->playerShader);

  const float alpha = services->GetInterpolationAlpha(services->context);
  const Vector2 renderPosition{
      state->playerPreviousPosition.x +
          (state->playerPosition.x - state->playerPreviousPosition.x) * alpha,

      state->playerPreviousPosition.y +
          (state->playerPosition.y - state->playerPreviousPosition.y) * alpha};

  const Font *font = services->GetFont(services->context, state->font);

  if (services->IsScreen(services->context, "Menu")) {
    ClearBackground(BLACK);

    DrawText("LEVYEKIT", 40, 40, 40, RAYWHITE);

    DrawText("Press ENTER to play", 40, 100, 24, LIGHTGRAY);

    if (font) {
      DrawTextEx(*font, "LevyeKit FontManager", Vector2{40.0f, 140.0f}, 32.0f,
                 1.0f, RAYWHITE);
    }

    return;
  }

  if (services->IsScreen(services->context, "Game")) {
    ClearBackground(DARKBLUE);

    DrawText(TextFormat("Fixed updates: %llu", static_cast<unsigned long long>(
                                                   state->fixedUpdateCount)),
             20, 200, 20, WHITE);

    if (playerTexture && playerShader) {
      BeginShaderMode(*playerShader);
      DrawTexture(*playerTexture, static_cast<int>(renderPosition.x),
                  static_cast<int>(renderPosition.y), WHITE);

      EndShaderMode();
    } else if (playerTexture) {
      DrawTexture(*playerTexture, static_cast<int>(renderPosition.x),
                  static_cast<int>(renderPosition.y), WHITE);
    } else {

      DrawCircleV(state->playerPosition, 30.0f, GOLD);
    }

    DrawText("Move: WASD / Arrows / Controller", 40, 40, 20, RAYWHITE);

    DrawText("ESC: Menu", 40, 70, 20, LIGHTGRAY);

    DrawText(TextFormat("Reloads: %i", state->reloadCount), 40, 100, 20,
             LIGHTGRAY);
  }
}

void OnShutdown(Levye::GameState *state, const Levye::HostServices *services) {

  /*
   * Unlike OnBeforeReload, this callback only runs when the application is
   * actually shutting down. Persistent gameplay state is no longer needed
   * after this point.
   */
  (void)state;

  if (services->LogInfo)
    services->LogInfo("Sandbox shutting down.");
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
          .OnBeforeReload = OnBeforeReload,
          .OnAfterReload = OnAfterReload,
          .OnUpdate = OnUpdate,
          .OnFixedUpdate = OnFixedUpdate,
          .OnDraw = OnDraw,
          .OnShutdown = OnShutdown};
}