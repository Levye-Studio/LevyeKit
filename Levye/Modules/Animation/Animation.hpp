
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

    return api && api->version == ANIMATION_API_VERSION && api->IsAvailable &&
           api->IsAvailable(host->context);
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

  /** @brief Creates a host-owned pose by copying a model's skeleton; model is borrowed. */
  [[nodiscard]] static AnimationPoseHandle CreatePose(AnimationControllerHandle controller,
                                                      const Model* model) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->CreatePose && model ? api->CreatePose(host->context, controller, model)
                                           : INVALID_ANIMATION_POSE;
  }

  /** @brief Destroys a pose; destroying its controller also invalidates it. */
  static void DestroyPose(AnimationPoseHandle pose) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    if (api && api->DestroyPose) api->DestroyPose(host->context, pose);
  }

  /** @brief Checks a generation-safe pose handle. */
  [[nodiscard]] static bool IsPoseValid(AnimationPoseHandle pose) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->IsPoseValid && api->IsPoseValid(host->context, pose);
  }

  /** @brief Returns the number of bones copied into a pose. */
  [[nodiscard]] static int GetPoseBoneCount(AnimationPoseHandle pose) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->GetPoseBoneCount ? api->GetPoseBoneCount(host->context, pose) : 0;
  }

  /** @brief Finds a bone by name in a copied skeleton. */
  [[nodiscard]] static int FindPoseBone(AnimationPoseHandle pose, const char* name) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->FindPoseBone && name ? api->FindPoseBone(host->context, pose, name) : -1;
  }

  /** @brief Copies a bone name into a caller-owned buffer. */
  [[nodiscard]] static bool GetPoseBoneName(AnimationPoseHandle pose, int bone, char* output,
                                            int capacity) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->GetPoseBoneName && output && capacity > 0 &&
           api->GetPoseBoneName(host->context, pose, bone, output, capacity);
  }

  /** @brief Returns a bone's parent index, or -1 for a root or invalid index. */
  [[nodiscard]] static int GetPoseBoneParent(AnimationPoseHandle pose, int bone) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->GetPoseBoneParent ? api->GetPoseBoneParent(host->context, pose, bone) : -1;
  }

  /** @brief Samples the controller at its current clock, replacing all prior pose edits. */
  [[nodiscard]] static bool SamplePose(AnimationPoseHandle pose) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->SamplePose && api->SamplePose(host->context, pose);
  }

  /** @brief Reads a sampled bone in local or model space. */
  [[nodiscard]] static bool GetPoseBoneTransform(AnimationPoseHandle pose, int bone,
                                                 AnimationBoneSpace space,
                                                 AnimationBoneTransform& output) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->GetPoseBoneTransform &&
           api->GetPoseBoneTransform(host->context, pose, bone, space, &output);
  }

  /** @brief Edits a sampled bone; unchanged child locals follow their parent. */
  [[nodiscard]] static bool SetPoseBoneTransform(AnimationPoseHandle pose, int bone,
                                                 AnimationBoneSpace space,
                                                 const AnimationBoneTransform& value) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->SetPoseBoneTransform &&
           api->SetPoseBoneTransform(host->context, pose, bone, space, &value);
  }

  /** @brief Applies edited transforms to a matching live raylib model. */
  [[nodiscard]] static bool ApplyPose(AnimationPoseHandle pose, Model* model) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->ApplyPose && model && api->ApplyPose(host->context, pose, model);
  }

  [[nodiscard]] static bool SolveTwoBoneIK(AnimationPoseHandle pose, int root, int middle, int end,
                                           Vector3 target, Vector3 pole, float tolerance = 0.001f) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);

    const float targetData[3] = {target.x, target.y, target.z};

    const float poleData[3] = {pole.x, pole.y, pole.z};

    return api && api->SolveTwoBoneIK(host->context, pose, root, middle, end, targetData, poleData,
                                      tolerance);
  }

  /** @brief Blends the sampled local rotations toward a solved IK pose. */
  [[nodiscard]] static bool SolveTwoBoneIKWeighted(AnimationPoseHandle pose, int root, int middle,
                                                   int end, Vector3 target, Vector3 pole,
                                                   float weight, float tolerance = 0.001f) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    if (!api || !api->SolveTwoBoneIKWeighted) return false;
    const float targetData[3] = {target.x, target.y, target.z};
    const float poleData[3] = {pole.x, pole.y, pole.z};
    return api->SolveTwoBoneIKWeighted(host->context, pose, root, middle, end, targetData, poleData,
                                       weight, tolerance);
  }

  /** @brief Solves with bend limits and explicit continuity; reachability is returned separately.
   */
  [[nodiscard]] static bool SolveTwoBoneIKConstrained(AnimationPoseHandle pose, int root,
                                                      int middle, int end, Vector3 target,
                                                      Vector3 pole,
                                                      const AnimationIKConstraint& constraint,
                                                      float weight, AnimationIKResult& result,
                                                      float tolerance = 0.001f) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    if (!api || !api->SolveTwoBoneIKConstrained) return false;
    const float targetData[3] = {target.x, target.y, target.z};
    const float poleData[3] = {pole.x, pole.y, pole.z};
    return api->SolveTwoBoneIKConstrained(host->context, pose, root, middle, end, targetData,
                                          poleData, &constraint, weight, tolerance, &result);
  }

  /** @brief Applies model-space IK requests by id to one sampled pose; results match input order.
   */
  [[nodiscard]] static bool SolveIKChains(AnimationPoseHandle pose,
                                          const AnimationIKChainRequest* requests, int count,
                                          AnimationIKChainResult* results) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->SolveIKChains &&
           api->SolveIKChains(host->context, pose, requests, count, results);
  }

  /** @brief Applies model/local end-bone orientations after positional IK; results match input
   * order. */
  [[nodiscard]] static bool SolveOrientations(AnimationPoseHandle pose,
                                              const AnimationOrientationRequest* requests,
                                              int count, AnimationOrientationResult* results) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->SolveOrientations &&
           api->SolveOrientations(host->context, pose, requests, count, results);
  }

  /** @brief Constructs a contact orientation in a caller-chosen coordinate space. */
  [[nodiscard]] static bool SurfaceOrientation(Vector3 normal, Vector3 forward, Vector3 localUp,
                                               Vector3 localForward, Quaternion& output,
                                               const Quaternion* previous = nullptr) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    if (!api || !api->SurfaceOrientation) return false;
    const float normalData[3] = {normal.x, normal.y, normal.z};
    const float forwardData[3] = {forward.x, forward.y, forward.z};
    const float upData[3] = {localUp.x, localUp.y, localUp.z};
    const float contactForwardData[3] = {localForward.x, localForward.y, localForward.z};
    const float previousData[4] = {previous ? previous->x : 0, previous ? previous->y : 0,
                                   previous ? previous->z : 0, previous ? previous->w : 1};
    float result[4]{};
    if (!api->SurfaceOrientation(host->context, normalData, forwardData, upData, contactForwardData,
                                 previous ? previousData : nullptr, result))
      return false;
    output = {result[0], result[1], result[2], result[3]};
    return true;
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

    const auto* api = static_cast<const AnimationAPI*>(
        host->GetModuleAPI(host->context, ModuleID::Animation, ANIMATION_API_VERSION));
    return api && api->version == ANIMATION_API_VERSION ? api : nullptr;
  }
};

}  // namespace Levye
