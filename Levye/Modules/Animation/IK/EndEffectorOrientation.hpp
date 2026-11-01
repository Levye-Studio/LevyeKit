#pragma once

#include "../EditablePose.hpp"

namespace Levye {

/** @brief Stateless orientation operations. Call after positional IK and before ApplyPose.
 * Model-space requests use the pose's model axes; local requests use parent axes.
 * Batch conflicts are symmetric: duplicate ids and ancestor/descendant end bones are rejected.
 * Other valid requests succeed independently in ascending-id order. */
struct EndEffectorOrientation {
  [[nodiscard]] static bool Solve(EditablePose& pose, const AnimationOrientationRequest& request,
                                  AnimationOrientationResult& result);
  [[nodiscard]] static bool SolveBatch(EditablePose& pose,
                                       const AnimationOrientationRequest* requests, int count,
                                       AnimationOrientationResult* results);

  /** @brief Build a rotation mapping arbitrary local contact-up/forward axes to a surface.
   * Inputs and output use one consistent space. `previous` resolves degenerate forward
   * projections when provided. No world-up direction is assumed. */
  [[nodiscard]] static bool SurfaceOrientation(Vector3 normal, Vector3 forward, Vector3 localUp,
                                               Vector3 localForward, Quaternion& output,
                                               const Quaternion* previous = nullptr);
};
}  // namespace Levye
