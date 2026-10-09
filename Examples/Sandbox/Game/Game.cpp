#include <Levye/LevyeKit.hpp>
#include <Levye/Modules/Serialization/Serialization.hpp>
#include <Levye/Modules/Serialization/SerializationAPI.hpp>
#ifdef LEVYE_WITH_IMGUI
#include <imgui.h>

#include <Levye/Modules/ImGui/ImGui.hpp>
#endif
#include <new>

#include "SerializationReloadCheck.hpp"

namespace {

/**
 * @brief Small example of serializing game-owned data.
 *
 * The game defines which fields are serialized without depending on yaml-cpp.
 */
struct PlayerSave {
  std::string name = "Player";
  int score = 0;
  Vector2 position{};
};

template <typename Archive>
bool Serialize(Archive& archive, PlayerSave& player) {
  return archive.Field("name", player.name) &&
         archive.Field("score", player.score) &&
         archive.Field("position", player.position);
}

/**
 * @brief Demonstrates the custom serialization API with a small player save.
 */
void SavePlayerExample() {
  using namespace Levye;

  if (!Serialization::Available()) return;

  PlayerSave player;
  player.name = "Nesmy";
  player.score = 420;
  player.position = {640.0f, 360.0f};

  const DocumentHandle document = Serialization::Create();

  if (document == InvalidDocumentHandle) {
    Log::Error("Player save example: document creation failed.");
    return;
  }

  const bool saved = Serialization::Write(document, "player", player) &&
                     Serialization::Save(document, "player.yaml");

  if (saved) {
    Log::Info("Player save example written.");
  } else {
    Log::Error("Player save example failed.");
  }

  Serialization::Destroy(document);
}

struct GameState {
  Vector2 playerPosition{640.0f, 360.0f};

  float playerSpeed = 250.0f;

  int score = 1;

  Levye::AssetHandle playerTexture{};
  Levye::AssetHandle clickSound{};
  Levye::AssetHandle music{};
  // Handle to a serialization document owned by the host.
  //
  // Only the numeric handle lives in GameState. The YAML document itself
  // stays in SerializationService, which is not unloaded during hot reload.
  Levye::DocumentHandle persistentDocument = Levye::InvalidDocumentHandle;
};

void BindServices(const Levye::HostServices* services) {
  if (services)
    Levye::Services::Bind(services);
  else
    Levye::Services::Unbind();
}

GameState* GetState(void* state) { return static_cast<GameState*>(state); }

void InitializeState(void* state) { new (state) GameState{}; }

void DestroyState(void* state) { GetState(state)->~GameState(); }

void OnLoad(void* state) {
  GameState* game = GetState(state);
  // game->initialized = true;

  /*
   * Keyboard movement.
   *
   * Both WASD and arrow keys feed the same logical axes.
   */
  Levye::Input::BindKeyAxis("MoveX", KEY_A, KEY_D);

  Levye::Input::BindKeyAxis("MoveX", KEY_LEFT, KEY_RIGHT);

  Levye::Input::BindKeyAxis("MoveY", KEY_W, KEY_S);

  Levye::Input::BindKeyAxis("MoveY", KEY_UP, KEY_DOWN);

  /*
   * Controller movement.
   */
  Levye::Input::BindGamepadAxis("MoveX", 0, GAMEPAD_AXIS_LEFT_X);

  Levye::Input::BindGamepadAxis("MoveY", 0, GAMEPAD_AXIS_LEFT_Y);

  Levye::Input::BindKey("MoveUp", KEY_W);

  Levye::Input::BindKey("MoveUp", KEY_UP);

  Levye::Input::BindKey("MoveDown", KEY_S);

  Levye::Input::BindKey("MoveDown", KEY_DOWN);

  Levye::Input::BindKey("MoveLeft", KEY_A);

  Levye::Input::BindKey("MoveLeft", KEY_LEFT);

  Levye::Input::BindKey("MoveRight", KEY_D);

  Levye::Input::BindKey("MoveRight", KEY_RIGHT);

  Levye::Input::BindKey("Confirm", KEY_ENTER);
  Levye::Input::BindGamepadButton("Confirm", 0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);

  Levye::Input::BindKey("Back", KEY_ESCAPE);
  Levye::Input::BindGamepadButton("Back", 0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);

  Levye::Input::BindKey("TestSound", KEY_SPACE);
  Levye::Input::BindGamepadButton("TestSound", 0,
                                  GAMEPAD_BUTTON_RIGHT_FACE_DOWN);

  Levye::Input::BindKey("PauseMusic", KEY_M);

  Levye::Input::BindKey("ResumeMusic", KEY_R);

  Levye::Input::BindKey("NormalTime", KEY_ONE);

  Levye::Input::BindKey("SlowTime", KEY_TWO);

  Levye::Input::BindKey("FastTime", KEY_THREE);

  Levye::Input::BindKey("Pause", KEY_P);

  Levye::Input::BindKey("TestMouse", KEY_C);
  Levye::Input::BindMouseButton("TestMouse", MOUSE_BUTTON_LEFT);

  game->playerTexture = Levye::Assets::LoadTexture("player.png");

  game->clickSound = Levye::Audio::LoadSound("click.wav");

  game->music = Levye::Audio::LoadMusic("music.ogg");

  Levye::Audio::SetMusicVolume(game->music, 0.5f);

  Levye::Audio::PlayMusic(game->music);

  if (Levye::Serialization::Available()) {
    Levye::Log::Info("Serialization module available.");

    // Keep one small game-facing serialization example in the Sandbox.
    SavePlayerExample();

    // Persist only the handle; the host keeps the document across hot reloads.
    if (SandboxSerialization::Initialize(game->persistentDocument)) {
      Levye::Log::Info(
          "Hot-reload test: document created with scalars and arrays.");
    } else {
      Levye::Log::Error("Hot-reload test: failed to initialize document.");
    }
  } else {
    Levye::Log::Warning("Serialization module unavailable.");
  }

  Levye::Log::Info("Sandbox started.");
}

void OnAfterReload(void* state) {
  GameState* game = GetState(state);

  Levye::Log::Info("Sandbox reloaded.");

  // Optional modules may be disabled; that is not a reload failure.
  if (!Levye::Serialization::Available()) return;

  if (SandboxSerialization::AfterReload(game->persistentDocument,
                                        "serialization-reload-test.yaml")) {
    Levye::Log::Info(
        "Hot-reload test passed: same handle, values, arrays, modification and "
        "save/load.");
  } else {
    Levye::Log::Error(
        "Hot-reload test failed: persistence or save/load check.");
  }
}

void OnBeforeReload(void* state) {
  /*
   * This callback runs while the old game module is still loaded.
   *
   * Use it for temporary module-specific cleanup if needed, but do not
   * destroy persistent gameplay state or host-owned resources.
   */
  (void)state;

  Levye::Log::Info("Sandbox preparing for reload.");
}

void OnUpdate(void* state, float deltaTime) {
  GameState* game = GetState(state);

  if (Levye::Screen::Is("Menu")) {
    if (Levye::Input::IsPressed("Confirm")) {
      Levye::Screen::Set("Game");
    }

    return;
  }

  if (Levye::Input::IsPressed("TestMouse")) {
    Levye::Log::Info("Mouse pressed");
  }

  if (Levye::Input::IsReleased("TestMouse")) {
    Levye::Log::Info("Mouse released");
  }

  const float mouseWheel = Levye::Input::GetMouseWheel();
  const Vector2 mouseDelta = Levye::Input::GetMouseDelta();

  if (mouseWheel != 0.0f) {
    std::string wheelMSG = "Wheel: " + std::to_string(mouseWheel);
    Levye::Log::Info(wheelMSG.c_str());
  }

  if (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f) {
    std::string mouseMSG = "Mouse delta: x=" + std::to_string(mouseDelta.x) +
                           " y=" + std::to_string(mouseDelta.y);
    Levye::Log::Info(mouseMSG.c_str());
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

    if (Levye::Input::IsPressed("PauseMusic")) {
      Levye::Audio::PauseMusic(game->music);
    }
    if (Levye::Input::IsPressed("ResumeMusic")) {
      Levye::Audio::ResumeMusic(game->music);
    }

    if (Levye::Input::IsPressed("TestSound")) {
      Levye::Audio::PlaySound(game->clickSound);
    }
  }
}

void OnFixedUpdate(void* state, float fixedDeltaTime) {
  constexpr float playerSpeed = 300.0f;
}

void OnDraw(void* state) {
  GameState* game = GetState(state);
  const Texture2D* playerTexture =
      Levye::Assets::GetTexture(game->playerTexture);

  const float alpha = Levye::Time::InterpolationAlpha();

  if (Levye::Screen::Is("Menu")) {
    ClearBackground(BLACK);

    DrawText("LEVYEKIT", 40, 40, 40, RAYWHITE);

    DrawText("Press ENTER / Controller A to play", 40, 100, 24, LIGHTGRAY);

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

    DrawText("ESC / Controller B: Menu", 40, 70, 20, LIGHTGRAY);
    DrawText("P: Pause time | M: Pause music | R: Resume music", 40, 100, 20,
             LIGHTGRAY);

    DrawText(Levye::Time::IsPaused() ? "PAUSED" : "RUNNING", 20, 20, 20, WHITE);
  }

#ifdef LEVYE_WITH_IMGUI
  if (Levye::ImGuiModule::IsAvailable()) {
    ImGui::Begin("LevyeKit Debug");

    ImGui::Text("ImGui module registered!");
    ImGui::Text("Availability: true");
    ImGui::Text("Hot reload: true");

    ImGui::End();
  }
#endif
}

void OnShutdown(void* state) {
  GameState* game = GetState(state);

  // OnShutdown runs when the application is actually exiting,
  // not when the game library is being hot-reloaded.
  //
  // This is the appropriate place to release the persistent
  // document created in OnLoad().
  if (!SandboxSerialization::Shutdown(game->persistentDocument)) {
    Levye::Log::Error(
        "Hot-reload test: failed to destroy persistent document.");
  }

  Levye::Log::Info("Sandbox shutting down.");
}
}  // namespace

/**
 * @brief Returns the public API implemented by the Sandbox game module.
 *
 * This is the single exported entry point required by LevyeKit.
 *
 * extern "C" disables C++ name mangling so the host can reliably locate
 * this function using the symbol name "GetGameAPI".
 */
LEVYE_GAME_EXPORT const Levye::GameAPI* GetGameAPI() {
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
