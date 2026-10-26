
#pragma once

#include <Levye/Core/ModuleAPI.hpp>
#include <Levye/Core/Services.hpp>
#include <Levye/Modules/Animation/AnimationAPI.hpp>

namespace Levye {

/**
 * @brief Access to LevyeKit's optional Animation module.
 *
 * All controller state remains owned by the host.
 */
class Animation {
 public:
  /** @brief Returns whether a compatible host Animation API is available. */
  [[nodiscard]] static bool IsAvailable() noexcept {
    const HostServices* host = Services::Get();

    if (!host || !host->GetModuleAPI) {
      return false;
    }

    const auto* api = static_cast<const AnimationAPI*>(
        host->GetModuleAPI(host->context, ModuleID::Animation, ANIMATION_API_VERSION));

    return api && api->IsAvailable && api->IsAvailable(host->context);
  }

  /** @brief Creates a host-owned controller handle. */
  [[nodiscard]] static AnimationControllerHandle CreateController() {
    const HostServices* host = Services::Get();
    const AnimationAPI* api = GetAPI(host);

    if (!api || !api->CreateController) {
      return INVALID_ANIMATION_CONTROLLER;
    }

    return api->CreateController(host->context);
  }

  /** @brief Destroys a host-owned controller. */
  static void DestroyController(AnimationControllerHandle controller) {
    const HostServices* host = Services::Get();
    const AnimationAPI* api = GetAPI(host);

    if (api && api->DestroyController) {
      api->DestroyController(host->context, controller);
    }
  }

  /** @brief Checks whether a controller handle is live. */
  [[nodiscard]] static bool IsControllerValid(AnimationControllerHandle controller) {
    const HostServices* host = Services::Get();
    const AnimationAPI* api = GetAPI(host);

    return api && api->IsControllerValid && api->IsControllerValid(host->context, controller);
  }

  /** @brief Starts direct playback for the given duration. */
  [[nodiscard]] static bool Play(AnimationControllerHandle handle, float duration,
                                 bool looping = true) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);

    return api && api->Play && api->Play(host->context, handle, duration, looping);
  }

  /** @brief Stops a controller's playback. */
  [[nodiscard]] static bool Stop(AnimationControllerHandle handle) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);

    return api && api->Stop && api->Stop(host->context, handle);
  }

  /** @brief Pauses playback and its active crossfade. */
  [[nodiscard]] static bool Pause(AnimationControllerHandle handle) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);

    return api && api->Pause && api->Pause(host->context, handle);
  }

  /** @brief Resumes playback and its active crossfade. */
  [[nodiscard]] static bool Resume(AnimationControllerHandle handle) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);

    return api && api->Resume && api->Resume(host->context, handle);
  }

  /** @brief Sets a finite nonnegative playback multiplier. */
  [[nodiscard]] static bool SetSpeed(AnimationControllerHandle handle, float speed) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);

    return api && api->SetSpeed && api->SetSpeed(host->context, handle, speed);
  }

  /** @brief Reads the source playback clock during a crossfade. */
  [[nodiscard]] static bool GetPlaybackState(AnimationControllerHandle handle,
                                             AnimationPlaybackState& output) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);

    return api && api->GetPlaybackState && api->GetPlaybackState(host->context, handle, &output);
  }

  /** @brief Returns the number of animations in a file. */
  [[nodiscard]] static int GetAnimationCount(const char* path) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);

    return api && api->LoadAnimations ? api->LoadAnimations(host->context, path, nullptr, 0) : 0;
  }

  /** @brief Loads animations into a caller-owned handle buffer. */
  [[nodiscard]] static int LoadAnimations(const char* path, AnimationResourceHandle* output,
                                          int capacity) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);

    return api && api->LoadAnimations ? api->LoadAnimations(host->context, path, output, capacity)
                                      : 0;
  }

  /** @brief Unloads an animation and detaches its controllers. */
  [[nodiscard]] static bool UnloadAnimation(AnimationResourceHandle animation) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);

    return api && api->UnloadAnimation && api->UnloadAnimation(host->context, animation);
  }

  /** @brief Binds a loaded animation to a controller. */
  [[nodiscard]] static bool BindAnimation(AnimationControllerHandle controller,
                                          AnimationResourceHandle animation,
                                          float framesPerSecond = 60.0f) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);

    return api && api->BindAnimation &&
           api->BindAnimation(host->context, controller, animation, framesPerSecond);
  }

  /** @brief Starts playback of the bound animation. */
  [[nodiscard]] static bool PlayBound(AnimationControllerHandle controller, bool looping = true) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);

    return api && api->PlayBound && api->PlayBound(host->context, controller, looping);
  }

  /** @brief Returns the controller's bound animation. */
  [[nodiscard]] static AnimationResourceHandle GetBoundAnimation(
      AnimationControllerHandle controller) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);

    return api && api->GetBoundAnimation ? api->GetBoundAnimation(host->context, controller)
                                         : INVALID_ANIMATION_RESOURCE;
  }

  /** @brief Copies a loaded animation's name into a caller-owned buffer. */
  [[nodiscard]] static bool GetAnimationName(AnimationResourceHandle animation, char* output,
                                             int capacity) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);

    return api && api->GetAnimationName && output && capacity > 0 &&
           api->GetAnimationName(host->context, animation, output, capacity);
  }

  /** @brief Applies the current pose to a live raylib model without retaining its pointer.
   * @details Clips and model must share bone ordering; raylib 6.2 checks only bone count. */
  [[nodiscard]] static bool ApplyBoundAnimation(AnimationControllerHandle controller,
                                                Model* model) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);

    return api && api->ApplyBoundAnimation && model &&
           api->ApplyBoundAnimation(host->context, controller, model);
  }

  /** @brief Smoothly transitions to another animation. */
  [[nodiscard]] static bool CrossFade(AnimationControllerHandle controller,
                                      AnimationResourceHandle target, float duration = 0.3f) {
    const auto* host = Services::Get();
    const AnimationAPI* api = GetAPI(host);
    return api && api->CrossFade &&
           api->CrossFade(Services::Get()->context, controller, target, duration);
  }

  /** @brief Retrieves transition progress. */
  [[nodiscard]] static bool GetTransitionState(AnimationControllerHandle controller,
                                               AnimationTransitionState& output) {
    const auto* host = Services::Get();
    const AnimationAPI* api = GetAPI(host);
    return api && api->GetTransitionState &&
           api->GetTransitionState(Services::Get()->context, controller, &output);
  }

  /** @brief Registers a named animation state. */
  [[nodiscard]] static bool AddState(AnimationControllerHandle controller, const char* name,
                                     AnimationResourceHandle animation,
                                     float transitionDuration = 0.25f, float speed = 1.0f,
                                     bool looping = true, const char* returnState = nullptr,
                                     int priority = 0) {
    const auto* host = Services::Get();
    const AnimationAPI* api = GetAPI(host);
    if (!api || !api->AddState) return false;

    AnimationStateConfig config{};
    config.animation = animation;
    config.transitionDuration = transitionDuration;
    config.speed = speed;
    config.looping = looping;
    config.returnState = returnState;
    config.priority = priority;

    return api->AddState(Services::Get()->context, controller, name, &config);
  }

  /** @brief Removes a named animation state. */
  [[nodiscard]] static bool RemoveState(AnimationControllerHandle controller, const char* name) {
    const auto* host = Services::Get();
    const AnimationAPI* api = GetAPI(host);

    return api && api->RemoveState && api->RemoveState(Services::Get()->context, controller, name);
  }

  /** @brief Activates a state unless a higher-priority state is still active. */
  [[nodiscard]] static bool SetState(AnimationControllerHandle controller, const char* name) {
    const auto* host = Services::Get();
    const AnimationAPI* api = GetAPI(host);

    return api && api->SetState && api->SetState(Services::Get()->context, controller, name);
  }

  /** @brief Restarts a named animation state. */
  [[nodiscard]] static bool RestartState(AnimationControllerHandle controller, const char* name) {
    const auto* host = Services::Get();
    const AnimationAPI* api = GetAPI(host);

    return api && api->RestartState &&
           api->RestartState(Services::Get()->context, controller, name);
  }

  /** @brief Checks whether the current one-shot state finished. */
  [[nodiscard]] static bool IsStateFinished(AnimationControllerHandle controller) {
    const auto* host = Services::Get();
    const AnimationAPI* api = GetAPI(host);

    return api && api->IsStateFinished &&
           api->IsStateFinished(Services::Get()->context, controller);
  }

  /** @brief Copies the active state name, or an empty string when no state is active. */
  [[nodiscard]] static bool GetCurrentState(AnimationControllerHandle controller, char* output,
                                            int capacity) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->GetCurrentState && output && capacity > 0 &&
           api->GetCurrentState(host->context, controller, output, capacity);
  }

  /** @brief Returns whether a crossfade is currently active. */
  [[nodiscard]] static bool IsTransitionActive(AnimationControllerHandle controller) {
    AnimationTransitionState state{};
    return GetTransitionState(controller, state) && state.active;
  }

  /** @brief Changes state regardless of priority, restarting it when already active. */
  [[nodiscard]] static bool ForceState(AnimationControllerHandle controller, const char* name) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->ForceState && name && api->ForceState(host->context, controller, name);
  }

  /** @brief Registers a rule with 1–64 conditions, copied by the host. */
  [[nodiscard]] static bool AddTransition(AnimationControllerHandle controller,
                                          const AnimationTransitionRule& rule) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->AddTransition && api->AddTransition(host->context, controller, &rule);
  }

  /** @brief Removes all conditional rules for a controller. */
  [[nodiscard]] static bool ClearTransitions(AnimationControllerHandle controller) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->ClearTransitions && api->ClearTransitions(host->context, controller);
  }

  /** @brief Sets a Boolean parameter owned by the host. */
  [[nodiscard]] static bool SetBoolParameter(AnimationControllerHandle controller, const char* name,
                                             bool value) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->SetBoolParameter && name &&
           api->SetBoolParameter(host->context, controller, name, value);
  }

  /** @brief Sets a finite float parameter owned by the host. */
  [[nodiscard]] static bool SetFloatParameter(AnimationControllerHandle controller,
                                              const char* name, float value) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->SetFloatParameter && name &&
           api->SetFloatParameter(host->context, controller, name, value);
  }

  /** @brief Sets an integer parameter owned by the host. */
  [[nodiscard]] static bool SetIntParameter(AnimationControllerHandle controller, const char* name,
                                            int value) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->SetIntParameter && name &&
           api->SetIntParameter(host->context, controller, name, value);
  }

  /** @brief Arms a trigger until a matching rule takes effect. */
  [[nodiscard]] static bool SetTrigger(AnimationControllerHandle controller, const char* name) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->SetTrigger && name && api->SetTrigger(host->context, controller, name);
  }

  /** @brief Disarms a trigger. */
  [[nodiscard]] static bool ResetTrigger(AnimationControllerHandle controller, const char* name) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->ResetTrigger && name && api->ResetTrigger(host->context, controller, name);
  }

 private:
  Animation() = delete;

  [[nodiscard]] static const AnimationAPI* GetAPI(const HostServices* host) {
    if (!host || !host->GetModuleAPI) {
      return nullptr;
    }

    return static_cast<const AnimationAPI*>(
        host->GetModuleAPI(host->context, ModuleID::Animation, ANIMATION_API_VERSION));
  }
};

}  // namespace Levye
