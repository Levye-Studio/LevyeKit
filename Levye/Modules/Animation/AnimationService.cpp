
#include "AnimationService.hpp"

#include <raymath.h>

#include <Levye/Core/HostContext.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>
#include <vector>

namespace Levye {

namespace {

/** @brief Blends two raylib bone transforms. */
Transform BlendTransform(const Transform& source, const Transform& target, float alpha) {
  Transform result{};

  result.translation = Vector3Lerp(source.translation, target.translation, alpha);

  result.rotation = QuaternionSlerp(source.rotation, target.rotation, alpha);

  result.scale = Vector3Lerp(source.scale, target.scale, alpha);

  return result;
}

/** @brief Samples a keyframe pose at the playback time. */
Transform SampleBone(const ModelAnimation& animation, int bone, float time, float fps) {
  const float frame = std::clamp(time * fps, 0.0f, static_cast<float>(animation.keyframeCount - 1));

  const int first = static_cast<int>(std::floor(frame));
  const int second = std::min(first + 1, animation.keyframeCount - 1);

  const float alpha = frame - static_cast<float>(first);

  return BlendTransform(animation.keyframePoses[first][bone], animation.keyframePoses[second][bone],
                        alpha);
}

/** @brief Samples all local bone transforms at the given playback time. */
void SampleAnimationPose(const ModelAnimation& animation, const AnimationPlaybackState& playback,
                         float framesPerSecond, std::vector<Transform>& output) {
  output.resize(static_cast<std::size_t>(animation.boneCount));

  for (int bone = 0; bone < animation.boneCount; ++bone) {
    output[static_cast<std::size_t>(bone)] =
        SampleBone(animation, bone, playback.time, framesPerSecond);
  }
}

/** @brief Advances a playback clock without accessing model data. */
void AdvancePlayback(AnimationPlaybackState& state, float deltaTime) {
  if (!state.playing || state.paused || state.duration <= 0.0f) {
    return;
  }

  const double nextTime =
      static_cast<double>(state.time) + static_cast<double>(deltaTime) * state.speed;

  if (state.looping) {
    state.time = static_cast<float>(std::fmod(nextTime, state.duration));
  } else if (nextTime >= state.duration) {
    state.time = state.duration;
    state.playing = false;
  } else {
    state.time = static_cast<float>(nextTime);
  }
}

}  // namespace

bool AnimationService::Initialize() {
  if (m_Initialized) return true;

  m_Initialized = true;
  return true;
}

void AnimationService::Shutdown() {
  m_Resources.Clear();

  if (!m_Initialized) return;

  // Preserve generations so stale handles never revive.
  m_FreeSlots.clear();

  for (std::size_t i = 0; i < m_Controllers.size(); ++i) {
    ControllerSlot& slot = m_Controllers[i];

    if (slot.active) {
      slot.active = false;
      slot.playback = {};
      slot.targetPlayback = {};
      slot.transition = {};
      slot.animation = INVALID_ANIMATION_RESOURCE;
      slot.evaluatedPose.clear();
      slot.transitionSourcePose.clear();
      slot.useSourceSnapshot = false;
      slot.stateMachine.reset();

      if (slot.generation != std::numeric_limits<std::uint32_t>::max()) {
        ++slot.generation;
      }
    }

    // Permanently retire slots whose generations are exhausted.
    if (slot.generation != std::numeric_limits<std::uint32_t>::max()) {
      m_FreeSlots.push_back(static_cast<std::uint32_t>(i));
    }
  }

  m_Initialized = false;
}

AnimationControllerHandle AnimationService::MakeHandle(std::uint32_t index,
                                                       std::uint32_t generation) noexcept {
  return (static_cast<std::uint64_t>(generation) << 32) | (static_cast<std::uint64_t>(index) + 1);
}

std::uint32_t AnimationService::HandleIndex(AnimationControllerHandle handle) noexcept {
  return static_cast<std::uint32_t>(handle) - 1;
}

std::uint32_t AnimationService::HandleGeneration(AnimationControllerHandle handle) noexcept {
  return static_cast<std::uint32_t>(handle >> 32);
}

AnimationControllerHandle AnimationService::CreateController() {
  if (!m_Initialized) {
    return INVALID_ANIMATION_CONTROLLER;
  }

  const bool reuse = !m_FreeSlots.empty();
  if (!reuse && m_Controllers.size() >= std::numeric_limits<std::uint32_t>::max())
    return INVALID_ANIMATION_CONTROLLER;
  const auto index = reuse ? m_FreeSlots.back() : static_cast<std::uint32_t>(m_Controllers.size());
  const auto generation = reuse ? m_Controllers[index].generation : 1u;
  auto machine = std::make_unique<AnimationStateMachine>(MakeHandle(index, generation));
  if (reuse)
    m_FreeSlots.pop_back();
  else
    m_Controllers.emplace_back();

  ControllerSlot& slot = m_Controllers[index];

  slot.active = true;
  slot.playback = {};
  slot.targetPlayback = {};
  slot.transition = {};
  slot.animation = INVALID_ANIMATION_RESOURCE;
  slot.framesPerSecond = 60.0f;
  slot.evaluatedPose.clear();
  slot.transitionSourcePose.clear();
  slot.useSourceSnapshot = false;
  slot.stateMachine = std::move(machine);

  return MakeHandle(index, slot.generation);
}

void AnimationService::DestroyController(AnimationControllerHandle handle) {
  if (!IsControllerValid(handle)) return;

  const std::uint32_t index = HandleIndex(handle);
  ControllerSlot& slot = m_Controllers[index];

  slot.active = false;
  slot.playback = {};
  slot.targetPlayback = {};
  slot.transition = {};
  slot.animation = INVALID_ANIMATION_RESOURCE;
  slot.evaluatedPose.clear();
  slot.transitionSourcePose.clear();
  slot.useSourceSnapshot = false;
  slot.stateMachine.reset();

  // Retire the slot if incrementing its generation would wrap.
  if (slot.generation != std::numeric_limits<std::uint32_t>::max()) {
    ++slot.generation;
    m_FreeSlots.push_back(index);
  }
}

bool AnimationService::IsControllerValid(AnimationControllerHandle handle) const noexcept {
  if (!m_Initialized || handle == INVALID_ANIMATION_CONTROLLER) {
    return false;
  }

  const std::uint32_t index = HandleIndex(handle);

  if (index >= m_Controllers.size()) return false;

  const ControllerSlot& slot = m_Controllers[index];

  return slot.active && slot.generation == HandleGeneration(handle);
}

int AnimationService::LoadAnimations(const char* path, AnimationResourceHandle* output,
                                     int capacity) {
  if (!m_Initialized || !path || !*path || capacity < 0) {
    return 0;
  }

  // Query only: load temporarily to determine the count,
  // then let raylib release the temporary batch.
  if (!output) {
    int count = 0;
    ModelAnimation* animations = LoadModelAnimations(path, &count);

    if (animations) {
      UnloadModelAnimations(animations, count);
    }

    return count > 0 ? count : 0;
  }

  const auto handles = m_Resources.Load(path);
  if (handles.empty() || handles.size() > static_cast<std::size_t>(capacity)) {
    // The API requires an all-or-nothing result. Retire handles before returning.
    for (const auto handle : handles) {
      (void)UnloadAnimation(handle);
    }
    return 0;
  }

  for (std::size_t i = 0; i < handles.size(); ++i) {
    output[i] = handles[i];
  }

  return static_cast<int>(handles.size());
}

bool AnimationService::BindAnimation(AnimationControllerHandle controller,
                                     AnimationResourceHandle animation, float framesPerSecond) {
  if (!IsControllerValid(controller) || !std::isfinite(framesPerSecond) ||
      framesPerSecond <= 0.0f) {
    return false;
  }

  const ModelAnimation* resource = m_Resources.Get(animation);

  if (!resource || resource->keyframeCount <= 0) {
    return false;
  }

  ControllerSlot& slot = m_Controllers[HandleIndex(controller)];

  slot.animation = animation;
  slot.framesPerSecond = framesPerSecond;
  slot.playback = {};
  slot.targetPlayback = {};
  slot.transition = {};
  slot.evaluatedPose.clear();
  slot.transitionSourcePose.clear();
  slot.useSourceSnapshot = false;

  if (slot.stateMachine) slot.stateMachine->SetCurrentState("");

  return true;
}

bool AnimationService::PlayBound(AnimationControllerHandle controller, bool looping) {
  if (!IsControllerValid(controller)) return false;

  const ControllerSlot& slot = m_Controllers[HandleIndex(controller)];
  const ModelAnimation* resource = m_Resources.Get(slot.animation);

  if (!resource || resource->keyframeCount <= 0) {
    return false;
  }

  // N sampled poses span N - 1 intervals.
  const float duration = static_cast<float>(resource->keyframeCount - 1) / slot.framesPerSecond;

  // A single-keyframe animation is a static pose.
  const float effectiveDuration = duration > 0.0f ? duration : 1.0f / slot.framesPerSecond;

  return Play(controller, effectiveDuration, looping);
}

bool AnimationService::UnloadAnimation(AnimationResourceHandle animation) {
  if (!m_Resources.IsValid(animation)) return false;

  for (ControllerSlot& slot : m_Controllers) {
    if (!slot.active) continue;

    if (slot.stateMachine) {
      slot.stateMachine->RemoveStatesUsing(animation);
    }

    if (slot.transition.source == animation || slot.transition.target == animation) {
      slot.transition = {};
      slot.targetPlayback = {};
      slot.transitionSourcePose.clear();
      slot.useSourceSnapshot = false;
    }

    if (slot.animation == animation) {
      slot.animation = INVALID_ANIMATION_RESOURCE;
      slot.playback = {};
      slot.evaluatedPose.clear();
    }
  }

  return m_Resources.Unload(animation);
}

AnimationResourceHandle AnimationService::GetBoundAnimation(
    AnimationControllerHandle controller) const noexcept {
  if (!IsControllerValid(controller)) {
    return INVALID_ANIMATION_RESOURCE;
  }

  const AnimationResourceHandle animation = m_Controllers[HandleIndex(controller)].animation;

  return m_Resources.IsValid(animation) ? animation : INVALID_ANIMATION_RESOURCE;
}

bool AnimationService::GetAnimationName(AnimationResourceHandle animation, char* output,
                                        int capacity) const noexcept {
  if (!m_Initialized || !output || capacity <= 0) return false;

  const ModelAnimation* resource = m_Resources.Get(animation);
  if (!resource) return false;

  std::size_t length = 0;
  while (length < sizeof(resource->name) && resource->name[length] != '\0') ++length;
  if (length >= static_cast<std::size_t>(capacity)) return false;

  std::memcpy(output, resource->name, length);
  output[length] = '\0';
  return true;
}

bool AnimationService::ApplyBoundAnimation(AnimationControllerHandle controller, Model* model) {
  if (!model || !IsControllerValid(controller)) {
    return false;
  }

  ControllerSlot& slot = m_Controllers[HandleIndex(controller)];

  const ModelAnimation* source = m_Resources.Get(slot.animation);

  if (!source || source->keyframeCount <= 0 || !source->keyframePoses || !IsModelValid(*model) ||
      !model->skeleton.bones || !IsModelAnimationValid(*model, *source)) {
    return false;
  }

  // Normal animation playback.
  if (!slot.transition.active) {
    const float frame = std::clamp(slot.playback.time * slot.framesPerSecond, 0.0f,
                                   static_cast<float>(source->keyframeCount - 1));

    SampleAnimationPose(*source, slot.playback, slot.framesPerSecond, slot.evaluatedPose);

    UpdateModelAnimation(*model, *source, frame);
    return true;
  }

  // Resolve the target animation.
  const ModelAnimation* target = m_Resources.Get(slot.transition.target);

  if (!target || target->keyframeCount <= 0 || !target->keyframePoses ||
      target->boneCount != source->boneCount || !IsModelAnimationValid(*model, *target) ||
      slot.transition.duration <= 0.0f) {
    return false;
  }

  // Calculate the transition weight.
  const float alpha = std::clamp(slot.transition.elapsed / slot.transition.duration, 0.0f, 1.0f);

  const int boneCount = source->boneCount;

  // Evaluate and blend each bone's transform.
  std::vector<Transform> poses(static_cast<std::size_t>(boneCount));

  for (int bone = 0; bone < boneCount; ++bone) {
    Transform sourcePose{};

    if (slot.useSourceSnapshot) {
      // An interrupted transition starts from the
      // previously evaluated blended pose.
      if (slot.transitionSourcePose.size() != static_cast<std::size_t>(boneCount)) {
        return false;
      }

      sourcePose = slot.transitionSourcePose[static_cast<std::size_t>(bone)];
    } else {
      // A regular transition starts from the
      // currently playing source animation.
      sourcePose = SampleBone(*source, bone, slot.playback.time, slot.framesPerSecond);
    }

    const Transform targetPose =
        SampleBone(*target, bone, slot.targetPlayback.time, slot.framesPerSecond);

    poses[static_cast<std::size_t>(bone)] = BlendTransform(sourcePose, targetPose, alpha);
  }

  // Keep a copy of the exact pose produced by this transition.
  slot.evaluatedPose = poses;

  // Create a temporary one-keyframe animation.
  Transform* poseFrames[] = {poses.data()};

  ModelAnimation blended{};
  blended.boneCount = boneCount;
  blended.keyframeCount = 1;
  blended.keyframePoses = poseFrames;

  // raylib 6.2 stores skeleton data in Model,
  // not ModelAnimation.
  if (!IsModelAnimationValid(*model, blended)) {
    return false;
  }

  // Apply the blended pose using raylib's skinning.
  UpdateModelAnimation(*model, blended, 0.0f);

  return true;
}

bool AnimationService::Play(AnimationControllerHandle handle, float duration, bool looping) {
  if (!IsControllerValid(handle) || !std::isfinite(duration) || duration <= 0.0f) {
    return false;
  }

  ControllerSlot& slot = m_Controllers[HandleIndex(handle)];
  AnimationPlaybackState& state = slot.playback;

  state = {};
  state.duration = duration;
  state.speed = 1.0f;
  state.playing = true;
  state.looping = looping;

  slot.transition = {};
  slot.targetPlayback = {};
  slot.evaluatedPose.clear();
  slot.transitionSourcePose.clear();
  slot.useSourceSnapshot = false;
  if (slot.stateMachine) slot.stateMachine->SetCurrentState("");

  return true;
}

bool AnimationService::Stop(AnimationControllerHandle handle) {
  if (!IsControllerValid(handle)) return false;

  ControllerSlot& slot = m_Controllers[HandleIndex(handle)];

  slot.playback.time = 0.0f;
  slot.playback.playing = false;
  slot.playback.paused = false;

  slot.transition = {};
  slot.targetPlayback = {};

  slot.evaluatedPose.clear();
  slot.transitionSourcePose.clear();
  slot.useSourceSnapshot = false;
  if (slot.stateMachine) slot.stateMachine->SetCurrentState("");

  return true;
}

bool AnimationService::Pause(AnimationControllerHandle handle) {
  if (!IsControllerValid(handle)) return false;

  ControllerSlot& slot = m_Controllers[HandleIndex(handle)];
  AnimationPlaybackState& state = slot.playback;

  if (!state.playing && !slot.transition.active) return false;

  state.paused = true;

  if (slot.transition.active) {
    slot.targetPlayback.paused = true;
  }

  return true;
}

bool AnimationService::Resume(AnimationControllerHandle handle) {
  if (!IsControllerValid(handle)) return false;

  ControllerSlot& slot = m_Controllers[HandleIndex(handle)];
  AnimationPlaybackState& state = slot.playback;

  if (!state.playing && !slot.transition.active) return false;

  state.paused = false;

  if (slot.transition.active) {
    slot.targetPlayback.paused = false;
  }

  return true;
}

bool AnimationService::SetSpeed(AnimationControllerHandle handle, float speed) {
  if (!IsControllerValid(handle) || !std::isfinite(speed) || speed < 0.0f) {
    return false;
  }

  m_Controllers[HandleIndex(handle)].playback.speed = speed;

  ControllerSlot& slot = m_Controllers[HandleIndex(handle)];

  if (slot.transition.active) {
    slot.targetPlayback.speed = speed;
  }

  return true;
}

bool AnimationService::GetPlaybackState(AnimationControllerHandle handle,
                                        AnimationPlaybackState& output) const {
  if (!IsControllerValid(handle)) return false;

  output = m_Controllers[HandleIndex(handle)].playback;
  return true;
}

bool AnimationService::CrossFade(AnimationControllerHandle controller,
                                 AnimationResourceHandle target, float duration, bool looping) {
  if (!IsControllerValid(controller) || !std::isfinite(duration) || duration < 0.0f) {
    return false;
  }

  ControllerSlot& slot = m_Controllers[HandleIndex(controller)];

  const ModelAnimation* targetAnimation = m_Resources.Get(target);
  const ModelAnimation* sourceAnimation = m_Resources.Get(slot.animation);

  if (!targetAnimation || !sourceAnimation || targetAnimation->keyframeCount <= 0 ||
      sourceAnimation->keyframeCount <= 0 ||
      targetAnimation->boneCount != sourceAnimation->boneCount) {
    return false;
  }

  // Ignore repeated requests for the same destination.
  if (slot.transition.active) {
    if (slot.transition.target == target) {
      return true;
    }
  } else if (slot.animation == target) {
    return true;
  }

  const bool interrupting = slot.transition.active;

  // The cached pose is the most recently evaluated visual pose.
  // It must match the target skeleton.
  if (interrupting) {
    const std::size_t boneCount = static_cast<std::size_t>(targetAnimation->boneCount);
    if (slot.evaluatedPose.size() == boneCount) {
      slot.transitionSourcePose = slot.evaluatedPose;
    } else {
      // A transition may be interrupted before the game renders a pose.
      // Sample the current blend so the controller remains usable off-screen.
      const ModelAnimation* previousTarget = m_Resources.Get(slot.transition.target);
      if (!previousTarget || previousTarget->boneCount != targetAnimation->boneCount ||
          slot.transition.duration <= 0.0f ||
          (slot.useSourceSnapshot && slot.transitionSourcePose.size() != boneCount))
        return false;
      const float alpha =
          std::clamp(slot.transition.elapsed / slot.transition.duration, 0.0f, 1.0f);
      std::vector<Transform> snapshot(boneCount);
      for (std::size_t bone = 0; bone < boneCount; ++bone) {
        const Transform sourcePose = slot.useSourceSnapshot
                                         ? slot.transitionSourcePose[bone]
                                         : SampleBone(*sourceAnimation, static_cast<int>(bone),
                                                      slot.playback.time, slot.framesPerSecond);
        const Transform targetPose = SampleBone(*previousTarget, static_cast<int>(bone),
                                                slot.targetPlayback.time, slot.framesPerSecond);
        snapshot[bone] = BlendTransform(sourcePose, targetPose, alpha);
      }
      slot.transitionSourcePose = std::move(snapshot);
    }
    slot.useSourceSnapshot = true;
  } else {
    slot.transitionSourcePose.clear();
    slot.useSourceSnapshot = false;
  }

  // Preserve current playback properties.
  const float speed = slot.playback.speed;
  const bool paused = slot.playback.paused;

  const float targetDuration =
      static_cast<float>(targetAnimation->keyframeCount - 1) / slot.framesPerSecond;

  slot.targetPlayback = {};
  slot.targetPlayback.duration =
      targetDuration > 0.0f ? targetDuration : 1.0f / slot.framesPerSecond;

  slot.targetPlayback.speed = speed;
  slot.targetPlayback.playing = true;
  slot.targetPlayback.paused = paused;
  slot.targetPlayback.looping = looping;

  // Immediate transition.
  if (duration == 0.0f) {
    slot.animation = target;
    slot.playback = slot.targetPlayback;

    slot.transition = {};
    slot.targetPlayback = {};
    slot.transitionSourcePose.clear();
    slot.useSourceSnapshot = false;

    if (slot.stateMachine) slot.stateMachine->SetCurrentState("");
    return true;
  }

  slot.transition = {};
  slot.transition.source = slot.animation;
  slot.transition.target = target;
  slot.transition.duration = duration;
  slot.transition.active = true;

  if (slot.stateMachine) slot.stateMachine->SetCurrentState("");

  return true;
}

bool AnimationService::GetTransitionState(AnimationControllerHandle controller,
                                          AnimationTransitionState& output) const {
  if (!IsControllerValid(controller)) {
    return false;
  }

  output = m_Controllers[HandleIndex(controller)].transition;

  return true;
}

bool AnimationService::AddState(AnimationControllerHandle controller, const char* name,
                                const AnimationStateDefinition& definition) {
  if (!IsControllerValid(controller) || !name || !*name ||
      !m_Resources.IsValid(definition.animation)) {
    return false;
  }

  ControllerSlot& slot = m_Controllers[HandleIndex(controller)];
  if (!slot.stateMachine) return false;
  const auto* previous = slot.stateMachine->FindState(name);
  const bool restart =
      previous && slot.stateMachine->GetCurrentState() == name &&
      (previous->animation != definition.animation || previous->speed != definition.speed ||
       previous->looping != definition.looping);
  if (!slot.stateMachine->AddState(name, definition)) return false;
  return !restart || RestartState(controller, name);
}

bool AnimationService::GetCurrentState(AnimationControllerHandle controller, char* output,
                                       int capacity) const noexcept {
  if (!output || capacity <= 0 || !IsControllerValid(controller)) return false;
  const auto& machine = m_Controllers[HandleIndex(controller)].stateMachine;
  if (!machine) return false;
  const std::string& name = machine->GetCurrentState();
  if (name.size() >= static_cast<std::size_t>(capacity)) return false;
  std::memcpy(output, name.c_str(), name.size() + 1);
  return true;
}

bool AnimationService::AddTransition(AnimationControllerHandle controller,
                                     const AnimationTransitionRule& rule) {
  if (!IsControllerValid(controller)) return false;
  auto& machine = m_Controllers[HandleIndex(controller)].stateMachine;
  return machine && machine->AddTransition(rule);
}

bool AnimationService::ClearTransitions(AnimationControllerHandle controller) {
  if (!IsControllerValid(controller)) return false;
  auto& machine = m_Controllers[HandleIndex(controller)].stateMachine;
  if (!machine) return false;
  machine->ClearTransitions();
  return true;
}

bool AnimationService::SetBoolParameter(AnimationControllerHandle controller, const char* name,
                                        bool value) {
  if (!IsControllerValid(controller) || !name) return false;
  auto& machine = m_Controllers[HandleIndex(controller)].stateMachine;
  return machine && machine->SetBoolParameter(name, value);
}

bool AnimationService::SetFloatParameter(AnimationControllerHandle controller, const char* name,
                                         float value) {
  if (!IsControllerValid(controller) || !name) return false;
  auto& machine = m_Controllers[HandleIndex(controller)].stateMachine;
  return machine && machine->SetFloatParameter(name, value);
}

bool AnimationService::SetIntParameter(AnimationControllerHandle controller, const char* name,
                                       int value) {
  if (!IsControllerValid(controller) || !name) return false;
  auto& machine = m_Controllers[HandleIndex(controller)].stateMachine;
  return machine && machine->SetIntParameter(name, value);
}

bool AnimationService::SetTrigger(AnimationControllerHandle controller, const char* name) {
  if (!IsControllerValid(controller) || !name) return false;
  auto& machine = m_Controllers[HandleIndex(controller)].stateMachine;
  return machine && machine->SetTrigger(name);
}

bool AnimationService::ResetTrigger(AnimationControllerHandle controller, const char* name) {
  if (!IsControllerValid(controller) || !name) return false;
  auto& machine = m_Controllers[HandleIndex(controller)].stateMachine;
  return machine && machine->ResetTrigger(name);
}

bool AnimationService::RemoveState(AnimationControllerHandle controller, const char* name) {
  if (!IsControllerValid(controller) || !name || !*name) {
    return false;
  }

  ControllerSlot& slot = m_Controllers[HandleIndex(controller)];
  if (!slot.stateMachine || !slot.stateMachine->FindState(name)) return false;
  if (slot.transition.active && slot.stateMachine->GetCurrentState() == name) {
    slot.transition = {};
    slot.targetPlayback = {};
    slot.transitionSourcePose.clear();
    slot.useSourceSnapshot = false;
  }
  return slot.stateMachine->RemoveState(name);
}

bool AnimationService::SetState(AnimationControllerHandle controller, const char* name) {
  return SetStateInternal(controller, name, false);
}

bool AnimationService::ForceState(AnimationControllerHandle controller, const char* name) {
  return SetStateInternal(controller, name, true);
}

bool AnimationService::SetStateInternal(AnimationControllerHandle controller, const char* name,
                                        bool force) {
  if (!IsControllerValid(controller) || !name || !*name) {
    return false;
  }

  ControllerSlot& slot = m_Controllers[HandleIndex(controller)];

  if (!slot.stateMachine) {
    return false;
  }

  const AnimationStateDefinition* definition = slot.stateMachine->FindState(name);

  if (!definition || !m_Resources.IsValid(definition->animation)) {
    return false;
  }

  // Repeated state requests should not restart playback.
  if (slot.stateMachine->GetCurrentState() == name) {
    return force ? RestartState(controller, name) : true;
  }

  const auto* current = slot.stateMachine->FindState(slot.stateMachine->GetCurrentState());
  const bool currentFinished = !slot.playback.looping && !slot.playback.playing &&
                               slot.playback.duration > 0.0f &&
                               slot.playback.time >= slot.playback.duration;
  if (!force && current && definition->priority < current->priority &&
      (slot.transition.active || !currentFinished))
    return false;

  const auto currentAnimation = GetBoundAnimation(controller);

  const bool sameResource =
      currentAnimation == definition->animation ||
      (slot.transition.active && slot.transition.target == definition->animation);
  if (currentAnimation == INVALID_ANIMATION_RESOURCE || sameResource) {
    // Starting a different named state on the same clip must apply its
    // playback settings and restart the clip, even without a crossfade.
    if (!BindAnimation(controller, definition->animation, slot.framesPerSecond) ||
        !PlayBound(controller, definition->looping)) {
      return false;
    }

    if (!SetSpeed(controller, definition->speed)) {
      return false;
    }
  } else {
    // Subsequent states use the existing crossfade system.
    if (!CrossFade(controller, definition->animation, definition->transitionDuration,
                   definition->looping)) {
      return false;
    }

    if (!SetSpeed(controller, definition->speed)) {
      return false;
    }
  }

  slot.stateMachine->SetCurrentState(name);
  return true;
}

bool AnimationService::RestartState(AnimationControllerHandle controller, const char* name) {
  if (!IsControllerValid(controller) || !name || !*name) {
    return false;
  }

  ControllerSlot& slot = m_Controllers[HandleIndex(controller)];

  if (!slot.stateMachine) {
    return false;
  }

  const AnimationStateDefinition* definition = slot.stateMachine->FindState(name);

  if (!definition || !m_Resources.IsValid(definition->animation)) {
    return false;
  }

  // If the requested state is different, use the
  // normal state transition system.
  if (slot.stateMachine->GetCurrentState() != name) {
    return SetState(controller, name);
  }

  // Restart the current state without a crossfade.
  // This is intentional for one-shot retriggering.
  if (!BindAnimation(controller, definition->animation, slot.framesPerSecond) ||
      !PlayBound(controller, definition->looping) || !SetSpeed(controller, definition->speed)) {
    return false;
  }

  slot.stateMachine->SetCurrentState(name);
  return true;
}

bool AnimationService::IsStateFinished(AnimationControllerHandle controller) const {
  if (!IsControllerValid(controller)) {
    return false;
  }

  const ControllerSlot& slot = m_Controllers[HandleIndex(controller)];

  if (!slot.stateMachine || slot.stateMachine->GetCurrentState().empty() ||
      slot.transition.active) {
    return false;
  }

  const AnimationPlaybackState& playback = slot.playback;

  return !playback.looping && !playback.playing && playback.duration > 0.0f &&
         playback.time >= playback.duration;
}

void AnimationService::Update(float deltaTime) {
  if (!m_Initialized || !std::isfinite(deltaTime) || deltaTime <= 0.0f) {
    return;
  }

  for (ControllerSlot& slot : m_Controllers) {
    if (!slot.active) continue;

    AdvancePlayback(slot.playback, deltaTime);

    if (!slot.transition.active) continue;

    AdvancePlayback(slot.targetPlayback, deltaTime);

    // A paused controller must also pause its transition.
    if (slot.playback.paused) continue;

    slot.transition.elapsed =
        std::min(slot.transition.elapsed + deltaTime, slot.transition.duration);

    if (slot.transition.elapsed >= slot.transition.duration) {
      slot.animation = slot.transition.target;
      slot.playback = slot.targetPlayback;

      slot.transition = {};
      slot.targetPlayback = {};

      slot.transitionSourcePose.clear();
      slot.useSourceSnapshot = false;
    }
  }

  UpdateStateMachines();
}

void AnimationService::UpdateStateMachines() {
  for (std::size_t index = 0; index < m_Controllers.size(); ++index) {
    ControllerSlot& slot = m_Controllers[index];

    if (!slot.active || !slot.stateMachine || slot.transition.active || slot.playback.paused) {
      continue;
    }

    const bool finished = !slot.playback.looping && !slot.playback.playing &&
                          slot.playback.duration > 0.0f &&
                          slot.playback.time >= slot.playback.duration;

    if (const auto* rule = slot.stateMachine->FindTransition(finished)) {
      const std::string target = rule->targetState;
      if (SetState(MakeHandle(index, slot.generation), target.c_str())) {
        slot.stateMachine->ConsumeTriggers(*rule);
        continue;
      }
    }

    if (!finished) continue;

    const std::string* returnState = slot.stateMachine->GetReturnState();

    if (!returnState) {
      continue;
    }

    // Copy before SetState: changing the state can
    // invalidate the pointer returned by GetReturnState().
    const std::string target = *returnState;

    const auto controller = MakeHandle(index, slot.generation);

    (void)ForceState(controller, target.c_str());
  }
}

namespace {

bool APIIsAvailable(void* context) {
  if (!context) return false;

  auto* host = static_cast<HostContext*>(context);

  return host->animation && host->animation->IsInitialized();
}

AnimationControllerHandle APICreateController(void* context) {
  if (!context) return INVALID_ANIMATION_CONTROLLER;

  auto* host = static_cast<HostContext*>(context);

  return host->animation ? host->animation->CreateController() : INVALID_ANIMATION_CONTROLLER;
}

void APIDestroyController(void* context, AnimationControllerHandle handle) {
  if (!context) return;

  auto* host = static_cast<HostContext*>(context);

  if (host->animation) {
    host->animation->DestroyController(handle);
  }
}

bool APIIsControllerValid(void* context, AnimationControllerHandle handle) {
  if (!context) return false;

  auto* host = static_cast<HostContext*>(context);

  return host->animation && host->animation->IsControllerValid(handle);
}

bool APIPlay(void* context, AnimationControllerHandle handle, float duration, bool looping) {
  if (!context) return false;

  auto* host = static_cast<HostContext*>(context);

  return host->animation && host->animation->Play(handle, duration, looping);
}

bool APIStop(void* context, AnimationControllerHandle handle) {
  if (!context) return false;

  auto* host = static_cast<HostContext*>(context);

  return host->animation && host->animation->Stop(handle);
}

bool APIPause(void* context, AnimationControllerHandle handle) {
  if (!context) return false;

  auto* host = static_cast<HostContext*>(context);

  return host->animation && host->animation->Pause(handle);
}

bool APIResume(void* context, AnimationControllerHandle handle) {
  if (!context) return false;

  auto* host = static_cast<HostContext*>(context);

  return host->animation && host->animation->Resume(handle);
}

bool APISetSpeed(void* context, AnimationControllerHandle handle, float speed) {
  if (!context) return false;

  auto* host = static_cast<HostContext*>(context);

  return host->animation && host->animation->SetSpeed(handle, speed);
}

bool APIGetPlaybackState(void* context, AnimationControllerHandle handle,
                         AnimationPlaybackState* output) {
  if (!context || !output) return false;

  auto* host = static_cast<HostContext*>(context);

  return host->animation && host->animation->GetPlaybackState(handle, *output);
}

int APILoadAnimations(void* context, const char* path, AnimationResourceHandle* output,
                      int capacity) {
  if (!context) return 0;

  auto* host = static_cast<HostContext*>(context);

  return host->animation ? host->animation->LoadAnimations(path, output, capacity) : 0;
}

bool APIUnloadAnimation(void* context, AnimationResourceHandle animation) {
  if (!context) return false;

  auto* host = static_cast<HostContext*>(context);

  return host->animation && host->animation->UnloadAnimation(animation);
}

bool APIBindAnimation(void* context, AnimationControllerHandle controller,
                      AnimationResourceHandle animation, float framesPerSecond) {
  if (!context) return false;

  auto* host = static_cast<HostContext*>(context);

  return host->animation && host->animation->BindAnimation(controller, animation, framesPerSecond);
}

bool APIPlayBound(void* context, AnimationControllerHandle controller, bool looping) {
  if (!context) return false;

  auto* host = static_cast<HostContext*>(context);

  return host->animation && host->animation->PlayBound(controller, looping);
}

AnimationResourceHandle APIGetBoundAnimation(void* context, AnimationControllerHandle controller) {
  if (!context) return INVALID_ANIMATION_RESOURCE;

  auto* host = static_cast<HostContext*>(context);

  return host->animation ? host->animation->GetBoundAnimation(controller)
                         : INVALID_ANIMATION_RESOURCE;
}

bool APIGetAnimationName(void* context, AnimationResourceHandle animation, char* output,
                         int capacity) {
  if (!context || !output) return false;
  auto* host = static_cast<HostContext*>(context);
  return host->animation && host->animation->GetAnimationName(animation, output, capacity);
}

bool APIApplyBoundAnimation(void* context, AnimationControllerHandle controller, Model* model) {
  if (!context || !model) return false;
  auto* host = static_cast<HostContext*>(context);
  return host->animation && host->animation->ApplyBoundAnimation(controller, model);
}

bool APICrossFade(void* context, AnimationControllerHandle controller,
                  AnimationResourceHandle target, float duration) {
  if (!context) return false;

  auto* host = static_cast<HostContext*>(context);

  return host->animation && host->animation->CrossFade(controller, target, duration);
}

bool APIGetTransitionState(void* context, AnimationControllerHandle controller,
                           AnimationTransitionState* output) {
  if (!context || !output) return false;

  auto* host = static_cast<HostContext*>(context);

  return host->animation && host->animation->GetTransitionState(controller, *output);
}

bool APIAddState(void* context, AnimationControllerHandle controller, const char* name,
                 const AnimationStateConfig* config) {
  if (!context || !name || !config) {
    return false;
  }

  auto* host = static_cast<HostContext*>(context);

  if (!host->animation) {
    return false;
  }

  AnimationStateDefinition definition{};
  definition.animation = config->animation;
  definition.transitionDuration = config->transitionDuration;
  definition.speed = config->speed;
  definition.looping = config->looping;
  definition.priority = config->priority;
  if (config->returnState) {
    definition.returnState = config->returnState;
  }

  return host->animation->AddState(controller, name, definition);
}

bool APIRemoveState(void* context, AnimationControllerHandle controller, const char* name) {
  if (!context || !name) {
    return false;
  }

  auto* host = static_cast<HostContext*>(context);

  return host->animation && host->animation->RemoveState(controller, name);
}

bool APISetState(void* context, AnimationControllerHandle controller, const char* name) {
  if (!context || !name) {
    return false;
  }

  auto* host = static_cast<HostContext*>(context);

  return host->animation && host->animation->SetState(controller, name);
}

bool APIRestartState(void* context, AnimationControllerHandle controller, const char* name) {
  if (!context || !name) return false;

  auto* host = static_cast<HostContext*>(context);

  return host->animation && host->animation->RestartState(controller, name);
}

bool APIIsStateFinished(void* context, AnimationControllerHandle controller) {
  if (!context) return false;

  auto* host = static_cast<HostContext*>(context);

  return host->animation && host->animation->IsStateFinished(controller);
}

bool APIGetCurrentState(void* context, AnimationControllerHandle controller, char* output,
                        int capacity) {
  if (!context || !output) return false;
  auto* host = static_cast<HostContext*>(context);
  return host->animation && host->animation->GetCurrentState(controller, output, capacity);
}

bool APIForceState(void* context, AnimationControllerHandle controller, const char* name) {
  if (!context || !name) return false;
  auto* host = static_cast<HostContext*>(context);
  return host->animation && host->animation->ForceState(controller, name);
}

bool APIAddTransition(void* context, AnimationControllerHandle controller,
                      const AnimationTransitionRule* rule) {
  if (!context || !rule) return false;
  auto* host = static_cast<HostContext*>(context);
  return host->animation && host->animation->AddTransition(controller, *rule);
}

bool APIClearTransitions(void* context, AnimationControllerHandle controller) {
  if (!context) return false;
  auto* host = static_cast<HostContext*>(context);
  return host->animation && host->animation->ClearTransitions(controller);
}

bool APISetBoolParameter(void* context, AnimationControllerHandle controller, const char* name,
                         bool value) {
  if (!context || !name) return false;
  auto* host = static_cast<HostContext*>(context);
  return host->animation && host->animation->SetBoolParameter(controller, name, value);
}

bool APISetFloatParameter(void* context, AnimationControllerHandle controller, const char* name,
                          float value) {
  if (!context || !name) return false;
  auto* host = static_cast<HostContext*>(context);
  return host->animation && host->animation->SetFloatParameter(controller, name, value);
}

bool APISetIntParameter(void* context, AnimationControllerHandle controller, const char* name,
                        int value) {
  if (!context || !name) return false;
  auto* host = static_cast<HostContext*>(context);
  return host->animation && host->animation->SetIntParameter(controller, name, value);
}

bool APISetTrigger(void* context, AnimationControllerHandle controller, const char* name) {
  if (!context || !name) return false;
  auto* host = static_cast<HostContext*>(context);
  return host->animation && host->animation->SetTrigger(controller, name);
}

bool APIResetTrigger(void* context, AnimationControllerHandle controller, const char* name) {
  if (!context || !name) return false;
  auto* host = static_cast<HostContext*>(context);
  return host->animation && host->animation->ResetTrigger(controller, name);
}

}  // namespace

AnimationAPI MakeAnimationAPI(AnimationService& service) {
  (void)service;

  return {.version = ANIMATION_API_VERSION,
          .IsAvailable = APIIsAvailable,
          .CreateController = APICreateController,
          .DestroyController = APIDestroyController,
          .IsControllerValid = APIIsControllerValid,
          .Play = APIPlay,
          .Stop = APIStop,
          .Pause = APIPause,
          .Resume = APIResume,
          .SetSpeed = APISetSpeed,
          .GetPlaybackState = APIGetPlaybackState,
          .LoadAnimations = APILoadAnimations,
          .UnloadAnimation = APIUnloadAnimation,
          .BindAnimation = APIBindAnimation,
          .PlayBound = APIPlayBound,
          .GetBoundAnimation = APIGetBoundAnimation,
          .GetAnimationName = APIGetAnimationName,
          .ApplyBoundAnimation = APIApplyBoundAnimation,
          .CrossFade = APICrossFade,
          .GetTransitionState = APIGetTransitionState,
          .AddState = APIAddState,
          .RemoveState = APIRemoveState,
          .SetState = APISetState,
          .RestartState = APIRestartState,
          .IsStateFinished = APIIsStateFinished,
          .GetCurrentState = APIGetCurrentState,
          .ForceState = APIForceState,
          .AddTransition = APIAddTransition,
          .ClearTransitions = APIClearTransitions,
          .SetBoolParameter = APISetBoolParameter,
          .SetFloatParameter = APISetFloatParameter,
          .SetIntParameter = APISetIntParameter,
          .SetTrigger = APISetTrigger,
          .ResetTrigger = APIResetTrigger};
}

}  // namespace Levye
