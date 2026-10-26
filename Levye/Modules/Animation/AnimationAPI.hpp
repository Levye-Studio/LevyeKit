#pragma once

#include <cstdint>

struct Model;

namespace Levye {

/**
 * @brief Version of the Animation module API.
 */
inline constexpr std::uint32_t ANIMATION_API_VERSION = 9;

/**
 * @brief Opaque handle to a host-owned animation controller.
 *
 * Zero represents an invalid handle.
 */
using AnimationControllerHandle = std::uint64_t;

inline constexpr AnimationControllerHandle INVALID_ANIMATION_CONTROLLER = 0;

/**
 * @brief Opaque handle to a host-owned animation resource.
 *
 * Zero represents an invalid handle.
 */
using AnimationResourceHandle = std::uint64_t;
inline constexpr AnimationResourceHandle INVALID_ANIMATION_RESOURCE = 0;

/** @brief Snapshot of a controller's source playback clock. */
struct AnimationPlaybackState {
  /** @brief Source clock position in seconds, including during a crossfade. */
  float time = 0.0f;
  /** @brief Source clip length in seconds. */
  float duration = 0.0f;
  /** @brief Nonnegative playback multiplier. */
  float speed = 1.0f;

  /** @brief Whether the source clock is advancing. */
  bool playing = false;
  /** @brief Whether playback and crossfade progress are paused. */
  bool paused = false;
  /** @brief Whether the source clock wraps at its duration. */
  bool looping = false;
};

/**
 * @brief Describes a transition between two animation resources.
 */
struct AnimationTransitionState {
  /** @brief Source resource while the crossfade is active. */
  AnimationResourceHandle source = INVALID_ANIMATION_RESOURCE;
  /** @brief Destination resource while the crossfade is active. */
  AnimationResourceHandle target = INVALID_ANIMATION_RESOURCE;

  /** @brief Elapsed crossfade time in seconds. */
  float elapsed = 0.0f;
  /** @brief Requested crossfade duration in seconds. */
  float duration = 0.0f;

  /** @brief True only while a crossfade is in progress. */
  bool active = false;
};

/** @brief Configuration for a named animation state. */
struct AnimationStateConfig {
  /** @brief Host-owned animation selected by this state. */
  AnimationResourceHandle animation = INVALID_ANIMATION_RESOURCE;

  /** @brief Crossfade duration in seconds when entering the state. */
  float transitionDuration = 0.25f;
  /** @brief Playback speed multiplier. */
  float speed = 1.0f;
  /** @brief Whether playback repeats after its final frame. */
  bool looping = true;

  /** @brief Optional state name used after a one-shot completes. */
  const char* returnState = nullptr;
  /** @brief Higher values resist interruption by lower-priority states. */
  int priority = 0;
};

/** @brief Type of a host-owned state-machine parameter. */
enum class AnimationParameterType : std::uint32_t { Bool, Float, Int, Trigger };

/** @brief Comparison used by a conditional state transition. */
enum class AnimationComparison : std::uint32_t {
  Equal,
  NotEqual,
  Greater,
  Less,
  GreaterOrEqual,
  LessOrEqual,
  Triggered
};

/** @brief One condition in a transition; all conditions must match. */
struct AnimationCondition {
  /** @brief Parameter name copied by the host at registration. */
  const char* parameter = nullptr;
  /** @brief Required parameter type. */
  AnimationParameterType type = AnimationParameterType::Bool;
  /** @brief Comparison applied to the parameter value. */
  AnimationComparison comparison = AnimationComparison::Equal;
  /** @brief Expected value when type is Bool. */
  bool boolValue = false;
  /** @brief Expected value when type is Float. */
  float floatValue = 0.0f;
  /** @brief Expected value when type is Int. */
  int intValue = 0;
};

/** @brief Conditional transition from one named state to another. */
struct AnimationTransitionRule {
  /** @brief Source state; null or empty matches any current state. */
  const char* sourceState = nullptr;
  /** @brief Existing destination state. */
  const char* targetState = nullptr;
  /** @brief Conditions copied by the host; all must match. */
  const AnimationCondition* conditions = nullptr;
  /** @brief Number of conditions; valid range is 1–64. */
  int conditionCount = 0;
  /** @brief Rule selection priority; ties keep registration order. */
  int priority = 0;
};

/**
 * @brief Host-provided interface for the optional Animation module.
 *
 * All operations use the opaque host context provided by
 * HostServices. No C++ containers or runtime objects cross
 * the hot-reload boundary. Calls are made on the host's main thread; pointers
 * passed to callbacks are borrowed only for the duration of each call.
 */
struct AnimationAPI {
  /** @brief Must match the version requested from the host. */
  std::uint32_t version = ANIMATION_API_VERSION;

  /** @brief Reports whether the host initialized the optional service. */
  bool (*IsAvailable)(void* context) = nullptr;

  /** @brief Creates a generation-safe, host-owned controller. */
  AnimationControllerHandle (*CreateController)(void* context) = nullptr;

  /** @brief Destroys a controller and invalidates its handle. */
  void (*DestroyController)(void* context, AnimationControllerHandle controller) = nullptr;

  /** @brief Checks whether a controller handle is live. */
  bool (*IsControllerValid)(void* context, AnimationControllerHandle controller) = nullptr;

  bool (*Play)(void* context, AnimationControllerHandle controller, float duration,
               bool looping) = nullptr;

  bool (*Stop)(void* context, AnimationControllerHandle controller) = nullptr;

  bool (*Pause)(void* context, AnimationControllerHandle controller) = nullptr;

  bool (*Resume)(void* context, AnimationControllerHandle controller) = nullptr;

  bool (*SetSpeed)(void* context, AnimationControllerHandle controller, float speed) = nullptr;

  bool (*GetPlaybackState)(void* context, AnimationControllerHandle controller,
                           AnimationPlaybackState* output) = nullptr;

  /** @brief Queries a file's clip count with null output, or loads all clips into output.
   * @details If capacity is insufficient, loads nothing and returns zero. */
  int (*LoadAnimations)(void* context, const char* path, AnimationResourceHandle* output,
                        int capacity) = nullptr;

  bool (*UnloadAnimation)(void* context, AnimationResourceHandle animation) = nullptr;

  bool (*BindAnimation)(void* context, AnimationControllerHandle controller,
                        AnimationResourceHandle animation, float framesPerSecond) = nullptr;

  bool (*PlayBound)(void* context, AnimationControllerHandle controller, bool looping) = nullptr;

  AnimationResourceHandle (*GetBoundAnimation)(void* context,
                                               AnimationControllerHandle controller) = nullptr;

  /** @brief Copies an animation's name into a caller-owned buffer. */
  bool (*GetAnimationName)(void* context, AnimationResourceHandle animation, char* output,
                           int capacity) = nullptr;

  /** @brief Applies a pose to a live raylib model; the host does not retain its pointer.
   * @details The caller must use clips with the model's bone ordering; raylib 6.2
   * only validates bone count. */
  bool (*ApplyBoundAnimation)(void* context, AnimationControllerHandle controller,
                              Model* model) = nullptr;

  /** @brief Starts a crossfade to another animation. */
  bool (*CrossFade)(void* context, AnimationControllerHandle controller,
                    AnimationResourceHandle target, float duration);

  /** @brief Retrieves the current transition state. */
  bool (*GetTransitionState)(void* context, AnimationControllerHandle controller,
                             AnimationTransitionState* output);

  /** @brief Registers or replaces a named animation state. */
  bool (*AddState)(void* context, AnimationControllerHandle controller, const char* name,
                   const AnimationStateConfig* config);

  /** @brief Removes a named animation state. */
  bool (*RemoveState)(void* context, AnimationControllerHandle controller, const char* name);

  /** @brief Activates a named animation state. */
  bool (*SetState)(void* context, AnimationControllerHandle controller, const char* name);

  /** @brief Restarts a named animation state. */
  bool (*RestartState)(void* context, AnimationControllerHandle controller, const char* name);

  /** @brief Checks whether the active one-shot state has completed. */
  bool (*IsStateFinished)(void* context, AnimationControllerHandle controller);

  /** @brief Copies the active state name into a caller-owned buffer. */
  bool (*GetCurrentState)(void* context, AnimationControllerHandle controller, char* output,
                          int capacity) = nullptr;

  /** @brief Forces a named state regardless of its priority. */
  bool (*ForceState)(void* context, AnimationControllerHandle controller,
                     const char* name) = nullptr;

  /** @brief Registers a conditional transition; ties use registration order. */
  bool (*AddTransition)(void* context, AnimationControllerHandle controller,
                        const AnimationTransitionRule* rule) = nullptr;

  /** @brief Removes all conditional transitions for a controller. */
  bool (*ClearTransitions)(void* context, AnimationControllerHandle controller) = nullptr;

  /** @brief Sets a Boolean parameter. */
  bool (*SetBoolParameter)(void* context, AnimationControllerHandle controller, const char* name,
                           bool value) = nullptr;
  /** @brief Sets a finite float parameter. */
  bool (*SetFloatParameter)(void* context, AnimationControllerHandle controller, const char* name,
                            float value) = nullptr;
  /** @brief Sets an integer parameter. */
  bool (*SetIntParameter)(void* context, AnimationControllerHandle controller, const char* name,
                          int value) = nullptr;
  /** @brief Arms a trigger until a matching transition consumes it. */
  bool (*SetTrigger)(void* context, AnimationControllerHandle controller,
                     const char* name) = nullptr;
  /** @brief Clears a trigger without changing other parameters. */
  bool (*ResetTrigger)(void* context, AnimationControllerHandle controller,
                       const char* name) = nullptr;
};
}  // namespace Levye
