#pragma once

#include <Levye/Debug/Log.hpp>
#include <Levye/Modules/Animation/Animation.hpp>
#include <cstdint>
#include <vector>

namespace Sandbox {

struct AnimationTest {
  Levye::AnimationControllerHandle controller = 0;
  Levye::AnimationResourceHandle animation = 0;
  std::uint32_t reloadCount = 0;
  bool initialized = false;
};

inline void InitializeAnimationTest(AnimationTest& test, const char* path) {
  if (!Levye::Animation::IsAvailable()) {
    Levye::Log::Warning("Animation module unavailable.");
    return;
  }

  test.controller = Levye::Animation::CreateController();

  test.initialized = Levye::Animation::IsControllerValid(test.controller);

  if (test.initialized) {
    Levye::Log::Info("Animation controller created.");
  } else {
    Levye::Log::Error("Animation controller creation failed.");
    return;
  }

  const int count = Levye::Animation::GetAnimationCount(path);
  if (count > 0) {
    std::vector<Levye::AnimationResourceHandle> handles(static_cast<std::size_t>(count));
    if (Levye::Animation::LoadAnimations(path, handles.data(), count) == count) {
      test.animation = handles[0];
      for (int i = 1; i < count; ++i) {
        (void)Levye::Animation::UnloadAnimation(handles[i]);
      }
    }
  }

  const bool bound = test.animation &&
                     Levye::Animation::BindAnimation(test.controller, test.animation, 60.0f);
  const bool playing = bound && Levye::Animation::PlayBound(test.controller, true);
  Levye::Log::Info(playing ? "PASS: persistent animation playback"
                           : "FAIL: persistent animation playback");

  if (playing) {
    const bool paused = Levye::Animation::Pause(test.controller);
    Levye::AnimationPlaybackState state{};
    const bool pauseState = Levye::Animation::GetPlaybackState(test.controller, state) && state.paused;
    const bool speed = Levye::Animation::SetSpeed(test.controller, 1.5f);
    const bool resumed = Levye::Animation::Resume(test.controller);
    const bool resumeState = Levye::Animation::GetPlaybackState(test.controller, state) &&
                             !state.paused && state.speed == 1.5f;
    Levye::Log::Info(paused && pauseState && speed && resumed && resumeState
                         ? "PASS: pause, resume, and speed"
                         : "FAIL: pause, resume, and speed");
  }
}

inline void VerifyAnimationReload(AnimationTest& test) {
  ++test.reloadCount;

  Levye::AnimationPlaybackState state{};
  if (Levye::Animation::IsControllerValid(test.controller) &&
      Levye::Animation::GetBoundAnimation(test.controller) == test.animation &&
      Levye::Animation::GetPlaybackState(test.controller, state) && state.playing) {
    Levye::Log::Info("PASS: controller and resource survived hot reload");
  } else {
    Levye::Log::Error("FAIL: controller or resource lost during hot reload");
  }
}

inline void ShutdownAnimationTest(AnimationTest& test) {
  if (!test.initialized) {
    return;
  }

  const auto oldHandle = test.controller;

  Levye::Animation::DestroyController(test.controller);

  if (Levye::Animation::IsControllerValid(oldHandle)) {
    Levye::Log::Error("Destroyed animation controller is still valid.");
  } else {
    Levye::Log::Info("Animation controller destroyed successfully.");
  }

  test.controller = 0;
  if (test.animation) {
    (void)Levye::Animation::UnloadAnimation(test.animation);
    test.animation = 0;
  }
  test.initialized = false;
}

inline void TestAnimationAPI(const char* path) {
  using namespace Levye;

  const int count = Animation::GetAnimationCount(path);

  if (count <= 0) {
    Log::Error("FAIL: No animations found");
    return;
  }

  std::vector<AnimationResourceHandle> handles(static_cast<std::size_t>(count));

  const int loaded = Animation::LoadAnimations(path, handles.data(), count);

  if (loaded != count) {
    Log::Error("FAIL: Animation loading");
    return;
  }

  const auto controller = Animation::CreateController();

  const bool bound = Animation::BindAnimation(controller, handles[0], 60.0f);

  const bool playing = bound && Animation::PlayBound(controller, true);

  const bool bindingValid = Animation::GetBoundAnimation(controller) == handles[0];
  char name[32]{};
  const bool named = Animation::GetAnimationName(handles[0], name, sizeof(name)) && name[0];

  Log::Info(bound ? "PASS: API v4 binding" : "FAIL: API v4 binding");

  Log::Info(playing ? "PASS: API v4 playback" : "FAIL: API v4 playback");

  Log::Info(bindingValid ? "PASS: API v4 handle lookup" : "FAIL: API v4 handle lookup");
  Log::Info(named ? "PASS: API v4 animation name" : "FAIL: API v4 animation name");

  const bool unloaded = Animation::UnloadAnimation(handles[0]);

  const bool detached = Animation::GetBoundAnimation(controller) == INVALID_ANIMATION_RESOURCE;

  Log::Info(unloaded && detached ? "PASS: API v4 resource detachment"
                                 : "FAIL: API v4 resource detachment");

  for (int i = 1; i < loaded; ++i) {
    (void)Animation::UnloadAnimation(handles[i]);
  }

  Animation::DestroyController(controller);
}

}  // namespace Sandbox
