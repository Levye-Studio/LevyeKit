
#pragma once

#include <raylib.h>
#include <raymath.h>

#include <algorithm>
#include <cmath>

/** @brief Helpers for raylib transforms, bone lookup, and skeleton compatibility. */
namespace Levye::AnimationUtils {

/** @brief Creates an identity transform. */
[[nodiscard]] inline Transform IdentityTransform() {
  return {.translation = {0.0f, 0.0f, 0.0f},
          .rotation = QuaternionIdentity(),
          .scale = {1.0f, 1.0f, 1.0f}};
}

/** @brief Interpolates between two transforms. */
[[nodiscard]] inline Transform BlendTransform(const Transform& a, const Transform& b,
                                              float weight) {
  weight = std::clamp(weight, 0.0f, 1.0f);

  Transform result{};

  result.translation = Vector3Lerp(a.translation, b.translation, weight);

  result.rotation = QuaternionNormalize(QuaternionSlerp(a.rotation, b.rotation, weight));

  result.scale = Vector3Lerp(a.scale, b.scale, weight);

  return result;
}

/** @brief Checks whether a bone index is valid. */
[[nodiscard]] inline bool IsValidBone(const Model& model, int boneIndex) {
  return boneIndex >= 0 && boneIndex < model.skeleton.boneCount && model.skeleton.bones != nullptr;
}

/** @brief Finds a bone by name, returning -1 if not found. */
[[nodiscard]] inline int FindBone(const Model& model, const char* name) {
  if (!name || !model.skeleton.bones) {
    return -1;
  }

  for (int i = 0; i < model.skeleton.boneCount; ++i) {
    if (TextIsEqual(model.skeleton.bones[i].name, name)) {
      return i;
    }
  }

  return -1;
}

/** @brief Checks whether an animation is compatible with a model. */
[[nodiscard]] inline bool IsCompatible(const Model& model, const ModelAnimation& animation) {
  return IsModelAnimationValid(model, animation);
}

}  // namespace Levye::AnimationUtils
