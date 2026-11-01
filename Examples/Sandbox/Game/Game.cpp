#include <Levye/LevyeKit.hpp>
#include <filesystem>
#include <new>

#ifdef LEVYE_WITH_ANIMATION
#include "AnimationScene.hpp"
#include "AnimationTest.hpp"
#endif
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
  return archive.Field("name", player.name) && archive.Field("score", player.score) &&
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

#ifdef LEVYE_WITH_ANIMATION
  Sandbox::AnimationTest animationTest;
  AnimationScene animationScene;
#endif
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
  Levye::Input::BindGamepadButton("TestSound", 0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);

  Levye::Input::BindKey("PauseMusic", KEY_M);

  Levye::Input::BindKey("ResumeMusic", KEY_R);

  Levye::Input::BindKey("NormalTime", KEY_ONE);

  Levye::Input::BindKey("SlowTime", KEY_TWO);

  Levye::Input::BindKey("FastTime", KEY_THREE);

  Levye::Input::BindKey("Pause", KEY_P);

#ifdef LEVYE_WITH_ANIMATION
  Levye::Input::BindKey("ShowAnimation", KEY_T);
  Levye::Input::BindKey("AnimationScene1", KEY_F1);
  Levye::Input::BindKey("AnimationScene2", KEY_F2);
  Levye::Input::BindKey("AnimationScene3", KEY_F3);
  Levye::Input::BindKey("AnimationScene4", KEY_F4);
  Levye::Input::BindKey("AnimationScene5", KEY_F5);
  Levye::Input::BindKey("AnimationScene6", KEY_F6);
  Levye::Input::BindKey("AnimationScene7", KEY_F7);
  Levye::Input::BindKey("AnimationNext", KEY_RIGHT);
  Levye::Input::BindKey("AnimationPrevious", KEY_LEFT);
  Levye::Input::BindKey("AnimationConfirm", KEY_ENTER);
  Levye::Input::BindKey("AnimationForce", KEY_F);
  Levye::Input::BindKey("AnimationPause", KEY_P);
  Levye::Input::BindKey("AnimationFaster", KEY_UP);
  Levye::Input::BindKey("AnimationSlower", KEY_DOWN);
  Levye::Input::BindKey("AnimationMove", KEY_W);
  Levye::Input::BindKey("AnimationRun", KEY_LEFT_SHIFT);
  Levye::Input::BindKey("AnimationDance", KEY_C);
  Levye::Input::BindKey("AnimationPunch", KEY_SPACE);
  Levye::Input::BindKey("AnimationJump", KEY_J);
  Levye::Input::BindKey("AnimationRestart", KEY_R);
  Levye::Input::BindKey("AnimationBoneNext", KEY_RIGHT_BRACKET);
  Levye::Input::BindKey("AnimationBonePrevious", KEY_LEFT_BRACKET);
  Levye::Input::BindKey("AnimationBonePositive", KEY_E);
  Levye::Input::BindKey("AnimationBoneNegative", KEY_Q);
  Levye::Input::BindKey("AnimationBoneReset", KEY_B);
  Levye::Input::BindKey("AnimationLeft", KEY_A);
  Levye::Input::BindKey("AnimationRight", KEY_D);
  Levye::Input::BindKey("AnimationDeath", KEY_D);
  Levye::Input::BindKey("AnimationWave", KEY_V);
  Levye::Input::BindKey("AnimationUnload", KEY_U);
  Levye::Input::BindKey("AnimationVerify", KEY_V);
  Levye::Input::BindKey("AnimationScene8", KEY_F8);
  Levye::Input::BindKey("AnimationScene9", KEY_F9);
  Levye::Input::BindKey("AnimationScene10", KEY_F10);
  Levye::Input::BindKey("AnimationScene11", KEY_F11);
  Levye::Input::BindKey("OrientationFlat", KEY_ONE);
  Levye::Input::BindKey("OrientationSlope", KEY_TWO);
  Levye::Input::BindKey("OrientationRotate", KEY_THREE);
  Levye::Input::BindKey("OrientationWeightDown", KEY_LEFT_BRACKET);
  Levye::Input::BindKey("OrientationWeightUp", KEY_RIGHT_BRACKET);
  Levye::Input::BindKey("OrientationWeightZero", KEY_ZERO);
  Levye::Input::BindKey("OrientationWeightFull", KEY_NINE);
  Levye::Input::BindKey("OrientationConstraints", KEY_C);
  Levye::Input::BindKey("OrientationFreeze", KEY_F);
  Levye::Input::BindKey("LocomotionBiped", KEY_ONE);
  Levye::Input::BindKey("LocomotionQuadruped", KEY_TWO);
  Levye::Input::BindKey("LocomotionHexapod", KEY_THREE);
  Levye::Input::BindKey("LocomotionWalk", KEY_G);
  Levye::Input::BindKey("LocomotionPause", KEY_P);
  Levye::Input::BindKey("LocomotionLeft", KEY_A);
  Levye::Input::BindKey("LocomotionRight", KEY_D);
  Levye::Input::BindKey("LocomotionForward", KEY_W);
  Levye::Input::BindKey("LocomotionBack", KEY_S);
  Levye::Input::BindKey("LocomotionTurnLeft", KEY_Q);
  Levye::Input::BindKey("LocomotionTurnRight", KEY_E);
  Levye::Input::BindKey("LocomotionSpeedDown", KEY_MINUS);
  Levye::Input::BindKey("LocomotionSpeedUp", KEY_EQUAL);
  Levye::Input::BindKey("LocomotionStrideDown", KEY_COMMA);
  Levye::Input::BindKey("LocomotionStrideUp", KEY_PERIOD);
  Levye::Input::BindKey("LocomotionMoveSpeedDown", KEY_SEMICOLON);
  Levye::Input::BindKey("LocomotionMoveSpeedUp", KEY_APOSTROPHE);
  Levye::Input::BindKey("LocomotionStabilization", KEY_B);
  Levye::Input::BindKey("LocomotionStrengthDown", KEY_N);
  Levye::Input::BindKey("LocomotionStrengthUp", KEY_H);
  Levye::Input::BindKey("LocomotionSupportDebug", KEY_V);
  Levye::Input::BindKey("LocomotionMarch", KEY_M);
  Levye::Input::BindKey("LocomotionHeightDown", KEY_LEFT_BRACKET);
  Levye::Input::BindKey("LocomotionHeightUp", KEY_RIGHT_BRACKET);
  Levye::Input::BindKey("ContactSelect", KEY_TAB);
  Levye::Input::BindKey("ContactToggle", KEY_C);
  Levye::Input::BindKey("ContactRelease", KEY_SPACE);
  Levye::Input::BindKey("ContactSurfacePrevious", KEY_LEFT);
  Levye::Input::BindKey("ContactSurfaceNext", KEY_RIGHT);

  Levye::Input::BindKey("IKToggle", KEY_I);
  Levye::Input::BindKey("IKTargetLeft", KEY_J);
  Levye::Input::BindKey("IKTargetRight", KEY_L);
  Levye::Input::BindKey("IKTargetForward", KEY_U);
  Levye::Input::BindKey("IKTargetBackward", KEY_O);
  Levye::Input::BindKey("IKTargetUp", KEY_K);
  Levye::Input::BindKey("IKTargetDown", KEY_M);
  Levye::Input::BindKey("IKPoleReverse", KEY_P);
  Levye::Input::BindKey("IKReset", KEY_R);
  Levye::Input::BindKey("IKWeightDown", KEY_LEFT_BRACKET);
  Levye::Input::BindKey("IKWeightUp", KEY_RIGHT_BRACKET);
  Levye::Input::BindKey("IKResponseDown", KEY_MINUS);
  Levye::Input::BindKey("IKResponseUp", KEY_EQUAL);
  Levye::Input::BindKey("IKConstraintToggle", KEY_ONE);
  Levye::Input::BindKey("IKMinDown", KEY_TWO);
  Levye::Input::BindKey("IKMinUp", KEY_THREE);
  Levye::Input::BindKey("IKMaxDown", KEY_FOUR);
  Levye::Input::BindKey("IKMaxUp", KEY_FIVE);
  Levye::Input::BindKey("IKSelectChain", KEY_TAB);
#endif

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
      Levye::Log::Info("Hot-reload test: document created with scalars and arrays.");
    } else {
      Levye::Log::Error("Hot-reload test: failed to initialize document.");
    }
  } else {
    Levye::Log::Warning("Serialization module unavailable.");
  }

#ifdef LEVYE_WITH_ANIMATION
  const auto robotPath = (std::filesystem::path(GetApplicationDirectory()) /
                          "../../../Examples/Sandbox/Assets/robot.glb")
                             .lexically_normal();
  Sandbox::InitializeAnimationTest(game->animationTest, robotPath.string().c_str());
  Sandbox::TestAnimationAPI(robotPath.string().c_str());
  if (!game->animationScene.Initialize(robotPath.string().c_str())) {
    Levye::Log::Error("Robot animation scenes initialization failed.");
  } else {
    Levye::Log::Info("Ten animation scenes ready.");
  }
#endif

  Levye::Log::Info("Sandbox started.");
}

void OnAfterReload(void* state) {
  GameState* game = GetState(state);

  Levye::Log::Info("Sandbox reloaded.");

  // Optional modules may be disabled; that is not a reload failure.
  // Each optional module must be tested independently.
  if (Levye::Serialization::Available()) {
    if (SandboxSerialization::AfterReload(game->persistentDocument,
                                          "serialization-reload-test.yaml")) {
      Levye::Log::Info("Hot-reload test passed: serialization state preserved. ");
    } else {
      Levye::Log::Error("Hot-reload test failed: serialization.");
    }
  }

  // Verify the host-owned controller after a successful module swap.
#ifdef LEVYE_WITH_ANIMATION
  Sandbox::VerifyAnimationReload(game->animationTest);
  Levye::Log::Info(game->animationScene.AfterReload()
                       ? "PASS: ten animation scenes survived hot reload"
                       : "FAIL: an animation scene lost state on hot reload");
#endif
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

#ifdef LEVYE_WITH_ANIMATION
  if (game->animationScene.IsReady()) {
    constexpr const char* sceneActions[AnimationScene::SceneCount] = {
        "AnimationScene1", "AnimationScene2",  "AnimationScene3", "AnimationScene4",
        "AnimationScene5", "AnimationScene6",  "AnimationScene7", "AnimationScene8",
        "AnimationScene9", "AnimationScene10", "AnimationScene11"};

    for (int scene = 0; scene < AnimationScene::SceneCount; ++scene) {
      if (Levye::Input::IsPressed(sceneActions[scene])) {
        if (game->animationScene.SelectScene(scene)) Levye::Screen::Set("Animation");
        return;
      }
    }
    if (Levye::Input::IsPressed("ShowAnimation")) {
      if (Levye::Screen::Is("Animation")) {
        (void)game->animationScene.SelectScene((game->animationScene.CurrentScene() + 1) %
                                               AnimationScene::SceneCount);
      } else {
        if (game->animationScene.SelectScene(0)) Levye::Screen::Set("Animation");
      }
      return;
    }
  }

  if (Levye::Screen::Is("Animation")) {
    if (Levye::Input::IsPressed("Back")) {
      Levye::Screen::Set("Menu");
    } else {
      game->animationScene.Update(deltaTime);
    }
    return;
  }
#endif

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
    std::string mouseMSG =
        "Mouse delta: x=" + std::to_string(mouseDelta.x) + " y=" + std::to_string(mouseDelta.y);
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

void OnFixedUpdate(void* state, float fixedDeltaTime) { constexpr float playerSpeed = 300.0f; }

void OnDraw(void* state) {
  GameState* game = GetState(state);

#ifdef LEVYE_WITH_ANIMATION
  if (Levye::Screen::Is("Animation")) {
    ClearBackground(DARKGRAY);
    game->animationScene.Draw();
    return;
  }
#endif

  const Texture2D* playerTexture = Levye::Assets::GetTexture(game->playerTexture);

  const float alpha = Levye::Time::InterpolationAlpha();

  if (Levye::Screen::Is("Menu")) {
    ClearBackground(BLACK);

    DrawText("LEVYEKIT", 40, 40, 40, RAYWHITE);

    DrawText("Press ENTER / Controller A to play", 40, 100, 24, LIGHTGRAY);

#ifdef LEVYE_WITH_ANIMATION
    if (game->animationScene.IsReady()) {
      DrawText("T: Animation scenes | F1-F11: Open a scene", 40, 135, 24, LIGHTGRAY);
    }

#endif

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
    DrawText("P: Pause time | M: Pause music | R: Resume music", 40, 100, 20, LIGHTGRAY);

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
    Levye::Log::Error("Hot-reload test: failed to destroy persistent document.");
  }

#ifdef LEVYE_WITH_ANIMATION
  game->animationScene.Shutdown();
  Sandbox::ShutdownAnimationTest(game->animationTest);
#endif

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
