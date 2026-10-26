#include "AnimationScene.hpp"

#include <Levye/Input/Input.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <vector>

namespace {
constexpr const char* SceneNames[AnimationScene::SceneCount] = {
    "Animation Playback",      "Crossfade Laboratory", "State Machine",
    "Conditional Transitions", "One-shot Actions",     "Combined Gameplay Simulation",
    "Lifecycle & Hot Reload"};

bool AddRule(Levye::AnimationControllerHandle controller, const char* source, const char* target,
             int priority, std::initializer_list<Levye::AnimationCondition> conditions) {
  const Levye::AnimationTransitionRule rule{source, target, conditions.begin(),
                                            static_cast<int>(conditions.size()), priority};
  return Levye::Animation::AddTransition(controller, rule);
}

Levye::AnimationCondition BoolCondition(const char* name, bool value) {
  Levye::AnimationCondition condition{};
  condition.parameter = name;
  condition.type = Levye::AnimationParameterType::Bool;
  condition.boolValue = value;
  return condition;
}

Levye::AnimationCondition FloatCondition(const char* name, Levye::AnimationComparison comparison,
                                         float value) {
  Levye::AnimationCondition condition{};
  condition.parameter = name;
  condition.type = Levye::AnimationParameterType::Float;
  condition.comparison = comparison;
  condition.floatValue = value;
  return condition;
}

Levye::AnimationCondition IntCondition(const char* name, int value) {
  Levye::AnimationCondition condition{};
  condition.parameter = name;
  condition.type = Levye::AnimationParameterType::Int;
  condition.intValue = value;
  return condition;
}

Levye::AnimationCondition TriggerCondition(const char* name) {
  Levye::AnimationCondition condition{};
  condition.parameter = name;
  condition.type = Levye::AnimationParameterType::Trigger;
  condition.comparison = Levye::AnimationComparison::Triggered;
  return condition;
}

bool IsOneShot(const char* name) {
  return std::strcmp(name, "Robot_Jump") == 0 || std::strcmp(name, "Robot_Punch") == 0 ||
         std::strcmp(name, "Robot_Death") == 0 || std::strcmp(name, "Robot_Wave") == 0;
}

int StatePriority(const char* name) {
  if (std::strcmp(name, "Robot_Death") == 0) return 10;
  if (std::strcmp(name, "Robot_Punch") == 0) return 5;
  if (std::strcmp(name, "Robot_Jump") == 0) return 4;
  if (std::strcmp(name, "Robot_Wave") == 0) return 3;
  if (std::strcmp(name, "Robot_Idle") == 0) return 1;
  return 1;
}
}  // namespace

const char* AnimationScene::SceneName(int index) const noexcept {
  return index >= 0 && index < SceneCount ? SceneNames[index] : "Unknown";
}

bool AnimationScene::SceneReady(int index) const noexcept {
  return m_Ready && index >= 0 && index < SceneCount &&
         Levye::Animation::IsControllerValid(m_Scenes[static_cast<std::size_t>(index)].controller);
}

int AnimationScene::FindClip(const char* name) const {
  if (!name) return -1;
  for (int i = 0; i < m_AnimationCount; ++i) {
    if (std::strcmp(m_Names[static_cast<std::size_t>(i)].data(), name) == 0) return i;
  }
  return -1;
}

const char* AnimationScene::ClipName(Levye::AnimationResourceHandle handle) const {
  for (int i = 0; i < m_AnimationCount; ++i) {
    if (m_Animations[static_cast<std::size_t>(i)] == handle)
      return m_Names[static_cast<std::size_t>(i)].data();
  }
  return "None";
}

bool AnimationScene::ConfigureConditional(Levye::AnimationControllerHandle controller) {
  using Levye::AnimationComparison;
  return AddRule(controller, nullptr, "Robot_Punch", 100, {TriggerCondition("Punch")}) &&
         AddRule(controller, nullptr, "Robot_Jump", 90, {TriggerCondition("Jump")}) &&
         AddRule(controller, "Robot_Idle", "Robot_Running", 20,
                 {BoolCondition("Moving", true),
                  FloatCondition("MoveSpeed", AnimationComparison::Greater, 0.7f)}) &&
         AddRule(controller, "Robot_Idle", "Robot_Walking", 10,
                 {BoolCondition("Moving", true),
                  FloatCondition("MoveSpeed", AnimationComparison::LessOrEqual, 0.7f)}) &&
         AddRule(controller, "Robot_Walking", "Robot_Running", 20,
                 {BoolCondition("Moving", true),
                  FloatCondition("MoveSpeed", AnimationComparison::Greater, 0.7f)}) &&
         AddRule(controller, "Robot_Running", "Robot_Walking", 10,
                 {BoolCondition("Moving", true),
                  FloatCondition("MoveSpeed", AnimationComparison::LessOrEqual, 0.7f)}) &&
         AddRule(controller, "Robot_Walking", "Robot_Idle", 10, {BoolCondition("Moving", false)}) &&
         AddRule(controller, "Robot_Running", "Robot_Idle", 10, {BoolCondition("Moving", false)}) &&
         AddRule(controller, "Robot_Idle", "Robot_Dance", 15, {IntCondition("Emote", 1)}) &&
         AddRule(controller, "Robot_Dance", "Robot_Idle", 10, {IntCondition("Emote", 0)});
}

bool AnimationScene::ConfigureController(int scene) {
  auto& view = m_Scenes[static_cast<std::size_t>(scene)];
  view.controller = Levye::Animation::CreateController();
  if (!Levye::Animation::IsControllerValid(view.controller)) return false;

  for (int i = 0; i < m_AnimationCount; ++i) {
    const char* name = m_Names[static_cast<std::size_t>(i)].data();
    const bool oneShot = IsOneShot(name);
    if (!Levye::Animation::AddState(view.controller, name,
                                    m_Animations[static_cast<std::size_t>(i)], 0.3f, 1.0f, !oneShot,
                                    oneShot ? "Robot_Idle" : nullptr, StatePriority(name)))
      return false;
  }

  if ((scene == 3 || scene == 5) && !ConfigureConditional(view.controller)) return false;

  const int idle = FindClip("Robot_Idle");
  if (idle < 0) return false;
  view.selectedClip = idle;
  if (scene == 0 || scene == 1) {
    return Levye::Animation::BindAnimation(view.controller,
                                           m_Animations[static_cast<std::size_t>(idle)]) &&
           Levye::Animation::PlayBound(view.controller);
  }
  return Levye::Animation::ForceState(view.controller, "Robot_Idle");
}

bool AnimationScene::Initialize(const char* modelPath) {
  if (m_Ready || !modelPath || !Levye::Animation::IsAvailable()) return false;
  if (std::strlen(modelPath) >= m_ModelPath.size()) return false;
  std::memcpy(m_ModelPath.data(), modelPath, std::strlen(modelPath) + 1);
  m_Model = LoadModel(modelPath);
  if (!IsModelValid(m_Model)) {
    Shutdown();
    return false;
  }

  const int count = Levye::Animation::GetAnimationCount(modelPath);
  if (count <= 0 || count > static_cast<int>(m_Animations.size()) ||
      Levye::Animation::LoadAnimations(modelPath, m_Animations.data(), count) != count) {
    Shutdown();
    return false;
  }
  m_AnimationCount = count;

  for (int i = 0; i < count; ++i) {
    if (!Levye::Animation::GetAnimationName(m_Animations[static_cast<std::size_t>(i)],
                                            m_Names[static_cast<std::size_t>(i)].data(),
                                            static_cast<int>(m_Names[0].size()))) {
      Shutdown();
      return false;
    }
  }
  for (const char* name : {"Robot_Idle", "Robot_Walking", "Robot_Running", "Robot_Dance",
                           "Robot_Jump", "Robot_Punch", "Robot_Death", "Robot_Wave"}) {
    if (FindClip(name) < 0) {
      Shutdown();
      return false;
    }
  }
  for (int scene = 0; scene < SceneCount; ++scene) {
    if (!ConfigureController(scene)) {
      Shutdown();
      return false;
    }
  }

  m_Camera.position = {10.0f, 6.0f, 12.0f};
  m_Camera.target = {0.0f, 1.5f, 0.0f};
  m_Camera.up = {0.0f, 1.0f, 0.0f};
  m_Camera.fovy = 45.0f;
  m_Camera.projection = CAMERA_PERSPECTIVE;
  m_Ready = true;
  return true;
}

bool AnimationScene::SelectScene(int index) {
  if (!SceneReady(index)) return false;
  m_CurrentScene = index;
  return true;
}

void AnimationScene::Record(int scene, bool success, const char* operation) {
  auto& view = m_Scenes[static_cast<std::size_t>(scene)];
  view.lastOperationPassed = success;
  std::snprintf(view.diagnostic.data(), view.diagnostic.size(), "%s: %s", operation,
                success ? "PASS" : "REJECTED/FAILED");
}

bool AnimationScene::SelectClip(int scene, int clip) {
  auto& view = m_Scenes[static_cast<std::size_t>(scene)];
  const int index = (clip % m_AnimationCount + m_AnimationCount) % m_AnimationCount;
  view.selectedClip = index;
  const auto handle = m_Animations[static_cast<std::size_t>(index)];
  if (scene == 0) {
    const bool success = Levye::Animation::BindAnimation(view.controller, handle) &&
                         Levye::Animation::PlayBound(view.controller);
    Record(scene, success, "Play selected clip");
    return success;
  }
  return true;
}

bool AnimationScene::SetNamedState(int scene, const char* name, bool force) {
  const auto controller = m_Scenes[static_cast<std::size_t>(scene)].controller;
  const bool success = force ? Levye::Animation::ForceState(controller, name)
                             : Levye::Animation::SetState(controller, name);
  Record(scene, success, force ? "Force state" : "Set state");
  return success;
}

void AnimationScene::UpdateParameters(int scene, float deltaTime) {
  auto& view = m_Scenes[static_cast<std::size_t>(scene)];
  const bool walking = Levye::Input::IsDown("AnimationMove");
  const bool left = scene == 5 && Levye::Input::IsDown("AnimationLeft");
  const bool right = scene == 5 && Levye::Input::IsDown("AnimationRight");
  const bool moving = walking || left || right;
  const float speed = moving ? (Levye::Input::IsDown("AnimationRun") ? 1.0f : 0.5f) : 0.0f;
  const int emote = Levye::Input::IsDown("AnimationDance") ? 1 : 0;
  bool success = Levye::Animation::SetBoolParameter(view.controller, "Moving", moving) &&
                 Levye::Animation::SetFloatParameter(view.controller, "MoveSpeed", speed) &&
                 Levye::Animation::SetIntParameter(view.controller, "Emote", emote);
  if (Levye::Input::IsPressed("AnimationPunch"))
    success = Levye::Animation::SetTrigger(view.controller, "Punch") && success;
  if (Levye::Input::IsPressed("AnimationJump"))
    success = Levye::Animation::SetTrigger(view.controller, "Jump") && success;
  if (!success) Record(scene, false, "Set conditions");
  if (scene == 5) {
    const float direction = static_cast<float>(right) - static_cast<float>(left);
    view.position = std::clamp(view.position + direction * speed * deltaTime * 3.0f, -6.0f, 6.0f);
  }
}

bool AnimationScene::RecreateLifecycleController() {
  auto& view = m_Scenes[6];
  const auto stale = view.controller;
  Levye::Animation::DestroyController(stale);
  const bool invalidated = !Levye::Animation::IsControllerValid(stale);
  view.controller = Levye::INVALID_ANIMATION_CONTROLLER;
  const bool recreated = ConfigureController(6);
  Record(6, invalidated && recreated, "Stale handle invalidation and recreation");
  return invalidated && recreated;
}

void AnimationScene::Update(float deltaTime) {
  if (!SceneReady(m_CurrentScene)) return;
  const int scene = m_CurrentScene;
  auto& view = m_Scenes[static_cast<std::size_t>(scene)];
  const auto controller = view.controller;

  if (scene == 0) {
    if (Levye::Input::IsPressed("AnimationPrevious"))
      (void)SelectClip(scene, view.selectedClip - 1);
    if (Levye::Input::IsPressed("AnimationNext")) (void)SelectClip(scene, view.selectedClip + 1);
    if (Levye::Input::IsPressed("AnimationRestart")) (void)SelectClip(scene, view.selectedClip);
  } else if (scene == 1) {
    if (Levye::Input::IsPressed("AnimationPrevious"))
      (void)SelectClip(scene, view.selectedClip - 1);
    if (Levye::Input::IsPressed("AnimationNext")) (void)SelectClip(scene, view.selectedClip + 1);
    if (Levye::Input::IsPressed("AnimationFaster"))
      view.crossfadeDuration = std::min(view.crossfadeDuration + 0.25f, 3.0f);
    if (Levye::Input::IsPressed("AnimationSlower"))
      view.crossfadeDuration = std::max(view.crossfadeDuration - 0.25f, 0.0f);
    if (Levye::Input::IsPressed("AnimationConfirm"))
      Record(scene,
             Levye::Animation::CrossFade(controller,
                                         m_Animations[static_cast<std::size_t>(view.selectedClip)],
                                         view.crossfadeDuration),
             "Crossfade to selected clip");
    if (Levye::Input::IsPressed("AnimationPunch")) {
      (void)SelectClip(scene, view.selectedClip + 1);
      Record(scene,
             Levye::Animation::CrossFade(controller,
                                         m_Animations[static_cast<std::size_t>(view.selectedClip)],
                                         view.crossfadeDuration),
             "Interrupt crossfade");
    }
    if (Levye::Input::IsPressed("AnimationRestart")) {
      const int idle = FindClip("Robot_Idle");
      Record(scene,
             idle >= 0 &&
                 Levye::Animation::BindAnimation(controller,
                                                 m_Animations[static_cast<std::size_t>(idle)]) &&
                 Levye::Animation::PlayBound(controller),
             "Reset to idle");
    }
  } else if (scene == 2) {
    if (Levye::Input::IsPressed("AnimationPrevious"))
      (void)SelectClip(scene, view.selectedClip - 1);
    if (Levye::Input::IsPressed("AnimationNext")) (void)SelectClip(scene, view.selectedClip + 1);
    if (Levye::Input::IsPressed("AnimationConfirm"))
      (void)SetNamedState(scene, m_Names[static_cast<std::size_t>(view.selectedClip)].data(),
                          false);
    if (Levye::Input::IsPressed("AnimationForce"))
      (void)SetNamedState(scene, m_Names[static_cast<std::size_t>(view.selectedClip)].data(), true);
    if (Levye::Input::IsPressed("AnimationRestart")) {
      char name[48]{};
      const bool success = Levye::Animation::GetCurrentState(controller, name, sizeof(name)) &&
                           name[0] && Levye::Animation::RestartState(controller, name);
      Record(scene, success, "Restart current state");
    }
  } else if (scene == 3 || scene == 5) {
    UpdateParameters(scene, deltaTime);
    if (Levye::Input::IsPressed("AnimationForce")) (void)SetNamedState(scene, "Robot_Idle", true);
    if (Levye::Input::IsPressed("AnimationRestart")) {
      char name[48]{};
      const bool success = Levye::Animation::GetCurrentState(controller, name, sizeof(name)) &&
                           name[0] && Levye::Animation::RestartState(controller, name);
      Record(scene, success, "Restart current state");
    }
  } else if (scene == 4) {
    if (Levye::Input::IsPressed("AnimationJump")) (void)SetNamedState(scene, "Robot_Jump", false);
    if (Levye::Input::IsPressed("AnimationPunch")) (void)SetNamedState(scene, "Robot_Punch", false);
    if (Levye::Input::IsPressed("AnimationDeath")) (void)SetNamedState(scene, "Robot_Death", false);
    if (Levye::Input::IsPressed("AnimationWave")) (void)SetNamedState(scene, "Robot_Wave", false);
    if (Levye::Input::IsPressed("AnimationConfirm"))
      (void)SetNamedState(scene, "Robot_Idle", false);
    if (Levye::Input::IsPressed("AnimationForce")) (void)SetNamedState(scene, "Robot_Idle", true);
    if (Levye::Input::IsPressed("AnimationRestart")) {
      char name[48]{};
      const bool success = Levye::Animation::GetCurrentState(controller, name, sizeof(name)) &&
                           name[0] && Levye::Animation::RestartState(controller, name);
      Record(scene, success, "Restart one-shot");
    }
  } else if (scene == 6) {
    if (Levye::Input::IsPressed("AnimationRestart")) (void)RecreateLifecycleController();
    if (Levye::Input::IsPressed("AnimationUnload")) {
      const int count = Levye::Animation::GetAnimationCount(m_ModelPath.data());
      std::vector<Levye::AnimationResourceHandle> temporary(
          static_cast<std::size_t>(std::max(count, 0)));
      bool success = count > 0 && Levye::Animation::LoadAnimations(
                                      m_ModelPath.data(), temporary.data(), count) == count;
      if (success) {
        const auto stale = temporary.front();
        for (auto handle : temporary)
          success = Levye::Animation::UnloadAnimation(handle) && success;
        char name[48]{};
        success = !Levye::Animation::GetAnimationName(stale, name, sizeof(name)) && success;
        success = Levye::Animation::IsControllerValid(controller) && success;
      }
      Record(scene, success, "Temporary resource unload");
    }
    if (Levye::Input::IsPressed("AnimationVerify"))
      Record(scene, VerifyPersistentHandles(), "Persistent handle verification");
  }

  if (Levye::Input::IsPressed("AnimationPause")) {
    Levye::AnimationPlaybackState playback{};
    const bool valid = Levye::Animation::GetPlaybackState(view.controller, playback);
    const bool success = valid && (playback.paused ? Levye::Animation::Resume(view.controller)
                                                   : Levye::Animation::Pause(view.controller));
    Record(scene, success, playback.paused ? "Resume" : "Pause");
  }
  if (scene == 0) {
    Levye::AnimationPlaybackState playback{};
    if (Levye::Animation::GetPlaybackState(view.controller, playback)) {
      if (Levye::Input::IsPressed("AnimationFaster"))
        Record(scene,
               Levye::Animation::SetSpeed(view.controller, std::min(playback.speed + 0.25f, 4.0f)),
               "Increase speed");
      if (Levye::Input::IsPressed("AnimationSlower"))
        Record(scene,
               Levye::Animation::SetSpeed(view.controller, std::max(playback.speed - 0.25f, 0.0f)),
               "Decrease speed");
    }
  }
  if (!Levye::Animation::ApplyBoundAnimation(view.controller, &m_Model))
    Record(scene, false, "Apply pose");
}

void AnimationScene::Draw() const {
  if (!SceneReady(m_CurrentScene)) return;
  const auto& view = m_Scenes[static_cast<std::size_t>(m_CurrentScene)];
  const auto controller = view.controller;
  const float x = m_CurrentScene == 5 ? view.position : 0.0f;
  BeginMode3D(m_Camera);
  DrawGrid(20, 1.0f);
  DrawModel(m_Model, {x, 0.0f, 0.0f}, 1.0f, WHITE);
  EndMode3D();

  char stateName[48]{};
  const bool named = Levye::Animation::GetCurrentState(controller, stateName, sizeof(stateName));
  Levye::AnimationPlaybackState playback{};
  const bool clock = Levye::Animation::GetPlaybackState(controller, playback);
  Levye::AnimationTransitionState transition{};
  const bool transitionValid = Levye::Animation::GetTransitionState(controller, transition);
  const auto bound = Levye::Animation::GetBoundAnimation(controller);
  const char* activeName = named && stateName[0] ? stateName : ClipName(bound);

  DrawRectangle(10, 10, 930, 360, Fade(BLACK, 0.78f));
  DrawText(TextFormat("%d/7  %s", m_CurrentScene + 1, SceneName(m_CurrentScene)), 20, 18, 26,
           RAYWHITE);
  DrawText("F1-F7: scenes | T: next scene | ESC: menu", 20, 54, 18, YELLOW);
  DrawText(TextFormat("State/clip: %s | Selected: %s", activeName,
                      m_Names[static_cast<std::size_t>(view.selectedClip)].data()),
           20, 84, 18, WHITE);
  DrawText(TextFormat("Controller: %s | Bound: %s | API: v%u",
                      Levye::Animation::IsControllerValid(controller) ? "valid" : "INVALID",
                      ClipName(bound), Levye::ANIMATION_API_VERSION),
           20, 110, 17, LIGHTGRAY);
  if (clock) {
    DrawText(TextFormat("Clock: %.2f / %.2f s | Speed: %.2fx | %s", playback.time,
                        playback.duration, playback.speed,
                        playback.paused    ? "PAUSED"
                        : playback.playing ? "PLAYING"
                                           : "FINISHED"),
             20, 136, 17, LIGHTGRAY);
  }
  if (transitionValid && transition.active) {
    const float progress =
        transition.duration > 0.0f ? 100.0f * transition.elapsed / transition.duration : 100.0f;
    DrawText(TextFormat("Crossfade: %s -> %s  %.0f%%", ClipName(transition.source),
                        ClipName(transition.target), progress),
             20, 162, 17, GREEN);
  } else {
    DrawText("Crossfade: inactive", 20, 162, 17, LIGHTGRAY);
  }
  DrawText(TextFormat("Last check: %s", view.diagnostic.data()), 20, 190, 17,
           view.lastOperationPassed ? GREEN : ORANGE);

  const char* controls = "";
  const char* detail = "";
  switch (m_CurrentScene) {
    case 0:
      controls = "LEFT/RIGHT: play clip | UP/DOWN: speed | P: pause | R: restart";
      detail = "Expected: clip and clock change; pause freezes time; speed changes rate.";
      break;
    case 1:
      controls = "LEFT/RIGHT: target | UP/DOWN: fade duration | ENTER: fade | SPACE: interrupt";
      detail = "P: pause | R: idle | Expected: blended pose and progress; interrupt stays smooth.";
      break;
    case 2:
      controls = "LEFT/RIGHT: target | ENTER: set | F: force | R: restart | P: pause";
      detail = "Expected: lower priority is rejected; force overrides; one-shots return.";
      break;
    case 3:
      controls = "W: walk | SHIFT+W: run | C: dance | SPACE: punch | J: jump";
      detail = "F: force idle | R: restart | Expected: typed conditions select states.";
      break;
    case 4:
      controls = "J: jump | SPACE: punch | D: death | V: wave | ENTER: request idle";
      detail = "F: force idle | R: restart | Expected: one-shots finish then return to idle.";
      break;
    case 5:
      controls = "A/D: move robot | W: walk | SHIFT: run | C: dance";
      detail = "SPACE: punch | J: jump | F: force idle | Expected: pose and position respond.";
      break;
    case 6:
      controls = "R: recreate controller | U: load/unload temporary clips | V: verify handles";
      detail = "Rebuild Game while running; reload count rises and state/resources persist.";
      break;
  }
  DrawText(controls, 20, 227, 17, YELLOW);
  DrawText(detail, 20, 252, 16, LIGHTGRAY);
  if (m_CurrentScene == 1)
    DrawText(TextFormat("Fade duration: %.2f s", view.crossfadeDuration), 20, 280, 17, WHITE);
  if (m_CurrentScene == 2 || m_CurrentScene == 4)
    DrawText(TextFormat("Current state priority: %d", StatePriority(activeName)), 20, 280, 17,
             WHITE);
  if (m_CurrentScene == 3 || m_CurrentScene == 5) {
    const bool moving = Levye::Input::IsDown("AnimationMove") ||
                        (m_CurrentScene == 5 && (Levye::Input::IsDown("AnimationLeft") ||
                                                 Levye::Input::IsDown("AnimationRight")));
    const float speed = moving ? (Levye::Input::IsDown("AnimationRun") ? 1.0f : 0.5f) : 0.0f;
    DrawText(
        TextFormat("Conditions: Moving=%s  MoveSpeed=%.1f  Emote=%d", moving ? "true" : "false",
                   speed, Levye::Input::IsDown("AnimationDance") ? 1 : 0),
        20, m_CurrentScene == 3 ? 280 : 307, 17, WHITE);
  }
  if (m_CurrentScene == 5)
    DrawText(TextFormat("Robot X position: %.2f", view.position), 20, 280, 17, WHITE);
  if (m_CurrentScene == 6)
    DrawText(TextFormat("Hot reloads observed: %u | Persistence: %s", m_ReloadCount,
                        m_ReloadPassed ? "PASS" : "FAIL"),
             20, 280, 17, m_ReloadPassed ? GREEN : RED);
  if (Levye::Animation::IsStateFinished(controller))
    DrawText("One-shot final frame reached", 20, 333, 17, GREEN);
}

bool AnimationScene::VerifyPersistentHandles() const {
  bool valid = m_Ready && IsModelValid(m_Model);
  for (int scene = 0; scene < SceneCount; ++scene) {
    const auto controller = m_Scenes[static_cast<std::size_t>(scene)].controller;
    char name[48]{};
    Levye::AnimationPlaybackState playback{};
    valid = Levye::Animation::IsControllerValid(controller) &&
            Levye::Animation::GetCurrentState(controller, name, sizeof(name)) &&
            Levye::Animation::GetPlaybackState(controller, playback) && valid;
  }
  for (int i = 0; i < m_AnimationCount; ++i) {
    char name[48]{};
    valid = Levye::Animation::GetAnimationName(m_Animations[static_cast<std::size_t>(i)], name,
                                               sizeof(name)) &&
            valid;
  }
  return valid;
}

bool AnimationScene::AfterReload() {
  ++m_ReloadCount;
  m_ReloadPassed = VerifyPersistentHandles();
  return m_ReloadPassed;
}

void AnimationScene::Shutdown() {
  for (auto& view : m_Scenes) {
    if (view.controller != Levye::INVALID_ANIMATION_CONTROLLER)
      Levye::Animation::DestroyController(view.controller);
    view = {};
  }
  for (int i = 0; i < m_AnimationCount; ++i)
    (void)Levye::Animation::UnloadAnimation(m_Animations[static_cast<std::size_t>(i)]);
  if (IsModelValid(m_Model)) UnloadModel(m_Model);
  m_Model = {};
  m_Animations.fill(Levye::INVALID_ANIMATION_RESOURCE);
  m_AnimationCount = 0;
  m_CurrentScene = 0;
  m_ReloadCount = 0;
  m_ReloadPassed = true;
  m_ModelPath.fill('\0');
  m_Ready = false;
}
