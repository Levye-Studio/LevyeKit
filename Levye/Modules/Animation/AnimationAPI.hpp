#pragma once

#include <cstdint>

struct Model;

namespace Levye {

/**
 * @brief Version of the Animation module API.
 */
inline constexpr std::uint32_t ANIMATION_API_VERSION = 15;

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

/** @brief Opaque host-owned pose; zero is invalid. Poses survive module reloads. */
using AnimationPoseHandle = std::uint64_t;
inline constexpr AnimationPoseHandle INVALID_ANIMATION_POSE = 0;

/** @brief Space used when reading or writing a bone transform. */
enum class AnimationBoneSpace : std::uint32_t { Local, Model };

/** @brief Plain-value bone transform, independent of raylib animation storage. */
struct AnimationBoneTransform {
  float translation[3] = {0.0f, 0.0f, 0.0f};
  float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  float scale[3] = {1.0f, 1.0f, 1.0f};
};

/** @brief Two-bone bend limits in radians: zero is straight, pi is fully folded. */
struct AnimationIKConstraint {
  float minBend = 0.0f;
  float maxBend = 3.14159265358979323846f;
  /** @brief Model-space preferred bend direction; zero permits pole/fallback selection. */
  float preferredDirection[3] = {0.0f, 0.0f, 0.0f};
  /** @brief Model-space prior bend direction, used when hasPreviousDirection is nonzero. */
  float previousDirection[3] = {0.0f, 0.0f, 0.0f};
  std::uint32_t hasPreviousDirection = 0;
};

/** @brief Result of a valid constrained solve; reachability describes full-weight geometry. */
struct AnimationIKResult {
  std::uint32_t reachable = 0;
  std::uint32_t usedPreviousDirection = 0;
  std::uint32_t usedFallbackDirection = 0;
  float bendAngle = 0.0f;
  float endError = 0.0f;
  float bendDirection[3] = {0.0f, 0.0f, 0.0f};
};

/** @brief Per-request batch outcome; input index and identifier are preserved. */
enum class AnimationIKChainStatus : std::uint32_t { Disabled, Invalid, Conflict, Solved, Failed };

/** @brief One model-space IK request. Identifiers must be unique within a batch. */
struct AnimationIKChainRequest {
  std::uint64_t id = 0;
  int root = -1;
  int middle = -1;
  int end = -1;
  float target[3] = {0.0f, 0.0f, 0.0f};
  float pole[3] = {0.0f, 0.0f, 0.0f};
  float weight = 1.0f;
  float tolerance = 0.001f;
  AnimationIKConstraint constraint{};
  std::uint32_t constrained = 0;
  std::uint32_t enabled = 1;
};

/** @brief Result at the same index as its request; `ik` is valid only when solved. */
struct AnimationIKChainResult {
  std::uint64_t id = 0;
  AnimationIKChainStatus status = AnimationIKChainStatus::Invalid;
  AnimationIKResult ik{};
};

/** @brief End rotation request; quaternions are xyzw and map local axes into the selected space.
 * Apply after positional IK. Angles are radians. A local contact offset is not preserved:
 * the end bone origin stays fixed while an offset point rotates around it. */
struct AnimationOrientationRequest {
  std::uint64_t id = 0;
  int bone = -1;
  float desired[4] = {0, 0, 0, 1};
  float weight = 1;
  float reference[4] = {0, 0, 0, 1};
  float twistAxis[3] = {0, 1, 0};
  float maxAngle = 3.14159265358979323846f;
  float maxTwist = 3.14159265358979323846f;
  AnimationBoneSpace space = AnimationBoneSpace::Model;
  std::uint32_t constrained = 0;
  std::uint32_t enabled = 1;
  /** @brief Optional surface normal and reference forward, in `space`. */
  float surfaceNormal[3] = {0, 1, 0};
  float referenceForward[3] = {0, 0, 1};
  float localContactUp[3] = {0, 1, 0};
  float localContactForward[3] = {0, 0, 1};
  float previous[4] = {0, 0, 0, 1};
  std::uint32_t hasSurface = 0;
  std::uint32_t hasPrevious = 0;
  /** @brief When set, desired/reference/surface vectors are world-space and converted
   * through worldFromModel. Only valid with `space == Model`. */
  float worldFromModel[4] = {0, 0, 0, 1};
  std::uint32_t worldSpace = 0;
};

enum class AnimationOrientationStatus : std::uint32_t {
  Disabled,
  Invalid,
  Conflict,
  Solved,
  Failed
};

/** @brief Result at the corresponding request index. Angular error is in radians. */
struct AnimationOrientationResult {
  std::uint64_t id = 0;
  AnimationOrientationStatus status = AnimationOrientationStatus::Invalid;
  float angularError = 0;
  std::uint32_t constrained = 0;
};

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

  /** @brief Copies model skeleton metadata; the model pointer is borrowed for this call. */
  AnimationPoseHandle (*CreatePose)(void* context, AnimationControllerHandle controller,
                                    const Model* model) = nullptr;
  /** @brief Destroys a pose and invalidates its generation-safe handle. */
  void (*DestroyPose)(void* context, AnimationPoseHandle pose) = nullptr;
  /** @brief Checks whether a pose and its controller are live. */
  bool (*IsPoseValid)(void* context, AnimationPoseHandle pose) = nullptr;
  /** @brief Returns the copied skeleton's bone count, or zero for an invalid pose. */
  int (*GetPoseBoneCount)(void* context, AnimationPoseHandle pose) = nullptr;
  /** @brief Finds a bone by its copied name; returns -1 when absent. */
  int (*FindPoseBone)(void* context, AnimationPoseHandle pose, const char* name) = nullptr;
  /** @brief Copies a bone name into a caller-owned buffer. */
  bool (*GetPoseBoneName)(void* context, AnimationPoseHandle pose, int bone, char* output,
                          int capacity) = nullptr;
  /** @brief Returns a bone's parent index, or -1 for a root or invalid index. */
  int (*GetPoseBoneParent)(void* context, AnimationPoseHandle pose, int bone) = nullptr;
  /** @brief Refreshes local transforms from the controller clock, discarding prior edits. */
  bool (*SamplePose)(void* context, AnimationPoseHandle pose) = nullptr;
  /** @brief Reads a sampled bone in local or model space. */
  bool (*GetPoseBoneTransform)(void* context, AnimationPoseHandle pose, int bone,
                               AnimationBoneSpace space, AnimationBoneTransform* output) = nullptr;
  /** @brief Sets one sampled bone; child model-space transforms propagate from it. */
  bool (*SetPoseBoneTransform)(void* context, AnimationPoseHandle pose, int bone,
                               AnimationBoneSpace space,
                               const AnimationBoneTransform* value) = nullptr;
  /** @brief Applies an edited pose to a matching live model using raylib skinning. */
  bool (*ApplyPose)(void* context, AnimationPoseHandle pose, Model* model) = nullptr;

  bool (*SolveTwoBoneIK)(void* context, AnimationPoseHandle pose, int root, int middle, int end,
                         const float target[3], const float pole[3], float tolerance) = nullptr;
  /** @brief Blends local joint rotations toward a solved two-bone IK pose. */
  bool (*SolveTwoBoneIKWeighted)(void* context, AnimationPoseHandle pose, int root, int middle,
                                 int end, const float target[3], const float pole[3], float weight,
                                 float tolerance) = nullptr;
  /** @brief Solves with bend limits and explicit pole continuity; weight 1 applies full IK. */
  bool (*SolveTwoBoneIKConstrained)(void* context, AnimationPoseHandle pose, int root, int middle,
                                    int end, const float target[3], const float pole[3],
                                    const AnimationIKConstraint* constraint, float weight,
                                    float tolerance, AnimationIKResult* result) = nullptr;
  /** @brief Applies independent requests by ascending id; returns per-request outcomes. */
  bool (*SolveIKChains)(void* context, AnimationPoseHandle pose,
                        const AnimationIKChainRequest* requests, int count,
                        AnimationIKChainResult* results) = nullptr;
  /** @brief Applies independent end rotations by ascending id after positional IK. */
  bool (*SolveOrientations)(void* context, AnimationPoseHandle pose,
                            const AnimationOrientationRequest* requests, int count,
                            AnimationOrientationResult* results) = nullptr;
  /** @brief Builds an orientation from a surface normal and contact axes in one space. */
  bool (*SurfaceOrientation)(void* context, const float normal[3], const float forward[3],
                             const float localUp[3], const float localForward[3],
                             const float previous[4], float output[4]) = nullptr;
};
}  // namespace Levye
