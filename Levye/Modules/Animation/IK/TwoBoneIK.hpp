
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "../EditablePose.hpp"

//
#include <raymath.h>

namespace Levye {

struct TwoBoneIK {
  static bool Solve(EditablePose& pose, int root, int middle, int end, Vector3 target, Vector3 pole,
                    float tolerance = 0.001f);
  /** @brief Blends local joint rotations toward a solved pose; failures leave the pose intact. */
  static bool SolveWeighted(EditablePose& pose, int root, int middle, int end, Vector3 target,
                            Vector3 pole, float weight, float tolerance = 0.001f);
  /** @brief Clamps bend to [min,max]; returns true for valid solves even when target is
   * unreachable. */
  static bool SolveConstrained(EditablePose& pose, int root, int middle, int end, Vector3 target,
                               Vector3 pole, const AnimationIKConstraint& constraint, float weight,
                               float tolerance, AnimationIKResult& result);

 private:
  static bool SolveInternal(EditablePose& pose, int root, int middle, int end, Vector3 target,
                            Vector3 pole, float tolerance, const AnimationIKConstraint* constraint,
                            AnimationIKResult* result);
  static bool Blend(EditablePose& pose, const EditablePose& solved, int root, int middle,
                    float weight);
  static bool Finite(Vector3 value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
  }

  static Vector3 Position(const AnimationBoneTransform& t) {
    return {t.translation[0], t.translation[1], t.translation[2]};
  }

  static Quaternion Rotation(const AnimationBoneTransform& t) {
    return {t.rotation[0], t.rotation[1], t.rotation[2], t.rotation[3]};
  }

  static void SetRotation(AnimationBoneTransform& t, Quaternion q) {
    q = QuaternionNormalize(q);

    t.rotation[0] = q.x;
    t.rotation[1] = q.y;
    t.rotation[2] = q.z;
    t.rotation[3] = q.w;
  }

  static bool RotationBetween(Vector3 from, Vector3 to, Quaternion& result);
};

}  // namespace Levye
