#include <Levye/LevyeKit.hpp>
#include <filesystem>
#include <new>

#include "GameState.hpp"
#include "Scenes/Input/InputScene.hpp"
#include "Scenes/Menu/MenuScene.hpp"
#include "Scenes/Serialization/SerializationScene.hpp"
#ifdef LEVYE_WITH_IMGUI
#include "Scenes/ImGui/ImGuiScene.hpp"
#endif
namespace {
using Sandbox::GameState;
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
  auto* game = GetState(state);
  Sandbox::InputScene::Bind();
#ifdef LEVYE_WITH_PHYSICS
  if (!game->physicsScene.Initialize()) Levye::Log::Error("Sandbox physics initialization failed.");
#endif
  game->core.Initialize();
  Sandbox::SerializationScene::Initialize(game->persistentDocument);
#ifdef LEVYE_WITH_ANIMATION
  const auto robotPath = (std::filesystem::path(GetApplicationDirectory()) /
                          "../../../Examples/Sandbox/Assets/robot.glb")
                             .lexically_normal();
  Sandbox::InitializeAnimationTest(game->animationTest, robotPath.string().c_str());
  Sandbox::TestAnimationAPI(robotPath.string().c_str());
  if (!game->animationScene.Initialize(robotPath.string().c_str()))
    Levye::Log::Error("Robot animation scenes initialization failed.");
  else
    Levye::Log::Info("Ten animation scenes ready.");
#endif
  Levye::Log::Info("Sandbox started.");
}
void OnBeforeReload(void* state) {
#ifdef LEVYE_WITH_PHYSICS
  GetState(state)->physicsScene.BeforeReload();
#endif
  Levye::Log::Info("Sandbox preparing for reload.");
}
void OnAfterReload(void* state) {
  auto* game = GetState(state);
  Levye::Log::Info("Sandbox reloaded.");
  Sandbox::SerializationScene::AfterReload(game->persistentDocument);
#ifdef LEVYE_WITH_ANIMATION
  Sandbox::VerifyAnimationReload(game->animationTest);
  Levye::Log::Info(game->animationScene.AfterReload()
                       ? "PASS: ten animation scenes survived hot reload"
                       : "FAIL: an animation scene lost state on hot reload");
#endif
#ifdef LEVYE_WITH_PHYSICS
  Levye::Log::Info(game->physicsScene.AfterReload() ? "PASS: Physics state survived hot reload"
                                                    : "FAIL: Physics state was not preserved");
#endif
}
void OnUpdate(void* state, float deltaTime) {
  auto* game = GetState(state);
#ifdef LEVYE_WITH_PHYSICS
  if (Levye::Input::IsPressed("ShowPhysics")) {
    Levye::Screen::Set("Physics");
    return;
  }
  if (Levye::Screen::Is("Physics")) {
    if (Levye::Input::IsPressed("Back"))
      Levye::Screen::Set("Menu");
    else
      game->physicsScene.Update();
    return;
  }
#endif
#ifdef LEVYE_WITH_ANIMATION
  if (game->animationScene.IsReady()) {
    constexpr const char* sceneActions[Sandbox::AnimationScene::SceneCount] = {
        "AnimationScene1", "AnimationScene2",  "AnimationScene3", "AnimationScene4",
        "AnimationScene5", "AnimationScene6",  "AnimationScene7", "AnimationScene8",
        "AnimationScene9", "AnimationScene10", "AnimationScene11"};
    for (int scene = 0; scene < Sandbox::AnimationScene::SceneCount; ++scene) {
      if (Levye::Input::IsPressed(sceneActions[scene])) {
        if (game->animationScene.SelectScene(scene)) Levye::Screen::Set("Animation");
        return;
      }
    }
    if (Levye::Input::IsPressed("ShowAnimation")) {
      if (Levye::Screen::Is("Animation"))
        (void)game->animationScene.SelectScene((game->animationScene.CurrentScene() + 1) %
                                               Sandbox::AnimationScene::SceneCount);
      else if (game->animationScene.SelectScene(0))
        Levye::Screen::Set("Animation");
      return;
    }
  }
  if (Levye::Screen::Is("Animation")) {
    if (Levye::Input::IsPressed("Back"))
      Levye::Screen::Set("Menu");
    else
      game->animationScene.Update(deltaTime);
    return;
  }
#endif
  if (Levye::Screen::Is("Menu")) {
    Sandbox::MenuScene::Update();
    return;
  }
  if (Levye::Screen::Is("Game")) game->core.Update(deltaTime);
}
void OnFixedUpdate(void*, float) {}
void OnDraw(void* state) {
  auto* game = GetState(state);
#ifdef LEVYE_WITH_PHYSICS
  if (Levye::Screen::Is("Physics")) {
    game->physicsScene.Draw();
    return;
  }
#endif
#ifdef LEVYE_WITH_ANIMATION
  if (Levye::Screen::Is("Animation")) {
    ClearBackground(DARKGRAY);
    game->animationScene.Draw();
    return;
  }
#endif
  if (Levye::Screen::Is("Menu")) {
#ifdef LEVYE_WITH_ANIMATION
    Sandbox::MenuScene::Draw(game->animationScene.IsReady());
#else
    Sandbox::MenuScene::Draw(false);
#endif
    return;
  }
  if (Levye::Screen::Is("Game")) game->core.Draw();
#ifdef LEVYE_WITH_IMGUI
  Sandbox::ImGuiScene::Draw();
#endif
}
void OnShutdown(void* state) {
  auto* game = GetState(state);
  Sandbox::SerializationScene::Shutdown(game->persistentDocument);
#ifdef LEVYE_WITH_ANIMATION
  game->animationScene.Shutdown();
  Sandbox::ShutdownAnimationTest(game->animationTest);
#endif
#ifdef LEVYE_WITH_PHYSICS
  game->physicsScene.Shutdown();
#endif
  Levye::Log::Info("Sandbox shutting downs.");
}
}  // namespace
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
