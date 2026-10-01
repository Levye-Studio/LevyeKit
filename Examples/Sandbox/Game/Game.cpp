#include "Levye/Input/Input.hpp"
#include <Levye/LevyeKit.hpp>

#include <new>

namespace {
struct GameState {
  Vector2 playerPosition{640.0f, 360.0f};

  float playerSpeed = 250.0f;

  int score = 1;

  Levye::AssetHandle playerTexture{};
  Levye::AssetHandle clickSound{};
  Levye::AssetHandle music{};
};

void BindServices(const Levye::HostServices *services) {
  if (services)
    Levye::Services::Bind(services);
  else
    Levye::Services::Unbind();
}

GameState *GetState(void *state) { return static_cast<GameState *>(state); }

void InitializeState(void *state) { new (state) GameState{}; }

void DestroyState(void *state) { GetState(state)->~GameState(); }

void OnLoad(void *state) {
  GameState *game = GetState(state);
  // game->initialized = true;

  game->playerTexture = Levye::Assets::LoadTexture("player.png");

  game->clickSound = Levye::Audio::LoadSound("click.wav");

  game->music = Levye::Audio::LoadMusic("music.ogg");

  Levye::Audio::SetMusicVolume(game->music, 0.5f);

  Levye::Audio::PlayMusic(game->music);

  Levye::Log::Info("Sandbox started.");
}

void OnAfterReload(void *state) {
  (void)state;

  Levye::Log::Info("Sandbox reloaded.");
}

void OnBeforeReload(void *state) {
  /*
   * This callback runs while the old game module is still loaded.
   *
   * Use it for temporary module-specific cleanup if needed, but do not
   * destroy persistent gameplay state or host-owned resources.
   */
  (void)state;

  Levye::Log::Info("Sandbox preparing for reload.");
}

void OnUpdate(void *state, float deltaTime) {

  GameState *game = GetState(state);

  if (Levye::Screen::Is("Menu")) {

    if (Levye::Input::IsPressed("Confirm")) {

      Levye::Screen::Set("Game");
    }

    return;
  }

  if (Levye::Screen::Is("Game")) {

    const float horizontal = Levye::Input::GetAxis("MoveX");
    const float vertical = Levye::Input::GetAxis("MoveY");

    game->playerPosition.x += horizontal * game->playerSpeed * deltaTime;
    game->playerPosition.y += vertical * game->playerSpeed * deltaTime;

    if (Levye::Input::IsPressed("Back")) {

      Levye::Screen::Set("Menu");

      return;
    }

    if (Levye::Input::IsPressed("NormalTime")) {
      Levye::Time::SetScale(1.0f);
    }

    if (Levye::Input::IsPressed("SlowTime")) {
      Levye::Time::SetScale(0.25f);
    }

    if (Levye::Input::IsPressed("FastTime")) {
      Levye::Time::SetScale(2.0f);
    }

    if (Levye::Input::IsPressed("Pause")) {
      const bool paused = Levye::Time::IsPaused();

      Levye::Time::SetPaused(!paused);
    }

    if (Levye::Input::IsPressed("TestSound")) {
      Levye::Audio::PlaySound(game->clickSound);
    }
  }
}

void OnFixedUpdate(void *state, float fixedDeltaTime) {

  constexpr float playerSpeed = 300.0f;
}

void OnDraw(void *state) {
  GameState *game = GetState(state);
  const Texture2D *playerTexture =
      Levye::Assets::GetTexture(game->playerTexture);

  const float alpha = Levye::Time::InterpolationAlpha();

  if (Levye::Screen::Is("Menu")) {
    ClearBackground(BLACK);

    DrawText("LEVYEKITs", 40, 40, 40, RAYWHITE);

    DrawText("Press ENTER to play", 40, 100, 24, LIGHTGRAY);

    return;
  }

  if (Levye::Screen::Is("Game")) {
    ClearBackground(DARKBLUE);

    if (playerTexture) {
      DrawTexture(*playerTexture, static_cast<int>(game->playerPosition.x),
                  static_cast<int>(game->playerPosition.y), WHITE);
    } else {

      DrawCircleV(game->playerPosition, 30.0f, GOLD);
    }

    DrawText("Move: WASD / Arrows / Controller", 40, 40, 20, RAYWHITE);

    DrawText("ESC: Menu", 40, 70, 20, LIGHTGRAY);

    DrawText(Levye::Time::IsPaused() ? "PAUSED" : "RUNNING", 20, 20, 20, WHITE);
  }
}

void OnShutdown(void *state) {

  /*
   * Unlike OnBeforeReload, this callback only runs when the application is
   * actually shutting down. Persistent gameplay state is no longer needed
   * after this point.
   */
  (void)state;

  Levye::Log::Info("Sandbox shutting down.");
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
LEVYE_GAME_EXPORT const Levye::GameAPI *GetGameAPI() {
  static const Levye::GameAPI api = {.version = Levye::GAME_API_VERSION,
                                     .BindServices = BindServices,

                                     .OnLoad = OnLoad,

                                     .OnBeforeReload = OnBeforeReload,
                                     .OnAfterReload = OnAfterReload,

                                     .OnUpdate = OnUpdate,
                                     .OnFixedUpdate = OnFixedUpdate,
                                     .OnDraw = OnDraw,
                                     .OnShutdown = OnShutdown,

                                     .InitializeState = InitializeState,
                                     .DestroyState = DestroyState,

                                     .stateSize = sizeof(GameState)};

  return &api;
}
