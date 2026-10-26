
#pragma once

#include <Levye/Modules/Animation/AnimationAPI.hpp>
#include <Levye/Modules/Animation/AnimationResource.hpp>
#include <Levye/Modules/Animation/AnimationStateMachine.hpp>
#include <cstdint>
#include <memory>
#include <vector>

namespace Levye {

/**
 * @brief Host-owned runtime for the optional Animation module.
 *
 * Controllers survive game library hot reloads and are
 * destroyed when the game session ends.
 */
class AnimationService {
 public:
  AnimationService() = default;
  ~AnimationService() = default;

  AnimationService(const AnimationService&) = delete;
  AnimationService& operator=(const AnimationService&) = delete;

  /** @brief Enables the host service; repeated initialization preserves state. */
  [[nodiscard]] bool Initialize();
  /** @brief Releases resources and invalidates all controller handles. */
  void Shutdown();

  /** @brief Returns whether the host service is active. */
  [[nodiscard]] bool IsInitialized() const noexcept { return m_Initialized; }

  /** @brief Creates a generation-safe, host-owned controller handle. */
  [[nodiscard]] AnimationControllerHandle CreateController();

  /** @brief Destroys a controller and invalidates its handle. */
  void DestroyController(AnimationControllerHandle handle);

  /** @brief Returns whether a generation-safe controller handle is live. */
  [[nodiscard]] bool IsControllerValid(AnimationControllerHandle handle) const noexcept;

  /** @brief Loads a whole file into caller-provided handles, or returns its count when output is
   * null. */
  [[nodiscard]] int LoadAnimations(const char* path, AnimationResourceHandle* output, int capacity);

  /** @brief Binds a loaded animation to a controller. */
  [[nodiscard]] bool BindAnimation(AnimationControllerHandle controller,
                                   AnimationResourceHandle animation,
                                   float framesPerSecond = 60.0f);

  /** @brief Plays the controller's bound animation. */
  [[nodiscard]] bool PlayBound(AnimationControllerHandle controller, bool looping = true);

  /** @brief Unloads an animation and detaches its controllers. */
  [[nodiscard]] bool UnloadAnimation(AnimationResourceHandle animation);

  /** @brief Returns the controller's bound animation handle. */
  [[nodiscard]] AnimationResourceHandle GetBoundAnimation(
      AnimationControllerHandle controller) const noexcept;

  /** @brief Copies a resource name without exposing its host-owned storage. */
  [[nodiscard]] bool GetAnimationName(AnimationResourceHandle animation, char* output,
                                      int capacity) const noexcept;

  /** @brief Applies the current pose to a caller-owned raylib model. */
  [[nodiscard]] bool ApplyBoundAnimation(AnimationControllerHandle controller, Model* model);

  /** @brief Starts direct playback and clears any named-state selection. */
  [[nodiscard]] bool Play(AnimationControllerHandle handle, float duration, bool looping);

  /** @brief Stops playback and clears any named-state selection. */
  [[nodiscard]] bool Stop(AnimationControllerHandle handle);
  /** @brief Pauses playback and an active crossfade. */
  [[nodiscard]] bool Pause(AnimationControllerHandle handle);
  /** @brief Resumes playback and an active crossfade. */
  [[nodiscard]] bool Resume(AnimationControllerHandle handle);

  /** @brief Sets a finite nonnegative playback speed. */
  [[nodiscard]] bool SetSpeed(AnimationControllerHandle handle, float speed);

  /** @brief Returns the source playback clock during a crossfade. */
  [[nodiscard]] bool GetPlaybackState(AnimationControllerHandle handle,
                                      AnimationPlaybackState& output) const;

  /** @brief Begins a timed transition to another animation. */
  [[nodiscard]] bool CrossFade(AnimationControllerHandle controller, AnimationResourceHandle target,
                               float duration, bool looping = true);

  /** @brief Returns the current transition state. */
  [[nodiscard]] bool GetTransitionState(AnimationControllerHandle controller,
                                        AnimationTransitionState& output) const;

  /** @brief Registers a named animation state for a controller. */
  [[nodiscard]] bool AddState(AnimationControllerHandle controller, const char* name,
                              const AnimationStateDefinition& definition);

  /** @brief Removes a named animation state. */
  [[nodiscard]] bool RemoveState(AnimationControllerHandle controller, const char* name);

  /** @brief Activates a named state using the configured transition. */
  [[nodiscard]] bool SetState(AnimationControllerHandle controller, const char* name);

  /** @brief Activates a named state regardless of priority. */
  [[nodiscard]] bool ForceState(AnimationControllerHandle controller, const char* name);

  /** @brief Copies the active state name without exposing host storage. */
  [[nodiscard]] bool GetCurrentState(AnimationControllerHandle controller, char* output,
                                     int capacity) const noexcept;

  /** @brief Adds a parameter-driven transition rule. */
  [[nodiscard]] bool AddTransition(AnimationControllerHandle controller,
                                   const AnimationTransitionRule& rule);
  /** @brief Removes all parameter-driven transition rules. */
  [[nodiscard]] bool ClearTransitions(AnimationControllerHandle controller);
  /** @brief Sets a typed Boolean parameter. */
  [[nodiscard]] bool SetBoolParameter(AnimationControllerHandle controller, const char* name,
                                      bool value);
  /** @brief Sets a typed finite float parameter. */
  [[nodiscard]] bool SetFloatParameter(AnimationControllerHandle controller, const char* name,
                                       float value);
  /** @brief Sets a typed integer parameter. */
  [[nodiscard]] bool SetIntParameter(AnimationControllerHandle controller, const char* name,
                                     int value);
  /** @brief Arms a trigger parameter. */
  [[nodiscard]] bool SetTrigger(AnimationControllerHandle controller, const char* name);
  /** @brief Disarms a trigger parameter. */
  [[nodiscard]] bool ResetTrigger(AnimationControllerHandle controller, const char* name);

  /** @brief Restarts a registered state from its first frame. */
  [[nodiscard]] bool RestartState(AnimationControllerHandle controller, const char* name);

  /** @brief Returns whether the current one-shot state has finished. */
  [[nodiscard]] bool IsStateFinished(AnimationControllerHandle controller) const;

  /** @brief Advances active controllers once per application frame. */
  void Update(float deltaTime);

  /** @brief Accesses host-owned animation resources. */
  [[nodiscard]] AnimationResource& GetResources() noexcept { return m_Resources; }

  /** @brief Accesses host-owned resources without allowing mutation. */
  [[nodiscard]] const AnimationResource& GetResources() const noexcept { return m_Resources; }

 private:
  struct ControllerSlot {
    std::uint32_t generation = 1;
    bool active = false;

    AnimationPlaybackState playback{};
    AnimationResourceHandle animation = INVALID_ANIMATION_RESOURCE;
    float framesPerSecond = 60.0f;

    AnimationTransitionState transition{};
    AnimationPlaybackState targetPlayback{};

    std::vector<Transform> evaluatedPose;
    std::vector<Transform> transitionSourcePose;
    bool useSourceSnapshot = false;

    std::unique_ptr<AnimationStateMachine> stateMachine;
  };

  std::vector<ControllerSlot> m_Controllers;
  std::vector<std::uint32_t> m_FreeSlots;

  AnimationResource m_Resources;
  bool m_Initialized = false;

  [[nodiscard]] static AnimationControllerHandle MakeHandle(std::uint32_t index,
                                                            std::uint32_t generation) noexcept;

  [[nodiscard]] static std::uint32_t HandleIndex(AnimationControllerHandle handle) noexcept;

  [[nodiscard]] static std::uint32_t HandleGeneration(AnimationControllerHandle handle) noexcept;

  /** @brief Processes completed one-shot state transitions. */
  void UpdateStateMachines();
  [[nodiscard]] bool SetStateInternal(AnimationControllerHandle controller, const char* name,
                                      bool force);
};

/** @brief Constructs the host Animation API function table. */
[[nodiscard]] AnimationAPI MakeAnimationAPI(AnimationService& service);

}  // namespace Levye
