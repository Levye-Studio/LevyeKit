
#include "TwoBoneIK.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>

namespace Levye {

bool TwoBoneIK::SolveWeighted(EditablePose& pose, int root, int middle, int end, Vector3 target,
                              Vector3 pole, float weight, float tolerance) {
  if (!std::isfinite(weight) || weight < 0.0f || weight > 1.0f || !Finite(target) ||
      !Finite(pole) || !std::isfinite(tolerance) || tolerance < 0.0f || !pose.IsSampled() ||
      pose.Parent(middle) != root || pose.Parent(end) != middle || root < 0 || middle < 0 ||
      end < 0)
    return false;

  EditablePose solved = pose;
  if (!SolveInternal(solved, root, middle, end, target, pole, tolerance, nullptr, nullptr))
    return false;
  EditablePose blended = pose;
  if (!Blend(blended, solved, root, middle, weight)) return false;
  pose = std::move(blended);
  return true;
}

bool TwoBoneIK::Blend(EditablePose& pose, const EditablePose& solved, int root, int middle,
                      float weight) {
  if (weight == 0.0f) return true;
  if (weight == 1.0f) {
    pose = solved;
    return true;
  }

  for (const int bone : {root, middle}) {
    AnimationBoneTransform original{};
    AnimationBoneTransform goal{};
    if (!pose.Get(bone, AnimationBoneSpace::Local, original) ||
        !solved.Get(bone, AnimationBoneSpace::Local, goal))
      return false;
    const Quaternion from = Rotation(original);
    const Quaternion to = Rotation(goal);
    SetRotation(original, QuaternionSlerp(from, to, weight));
    if (!pose.Set(bone, AnimationBoneSpace::Local, original)) return false;
  }
  return true;
}

bool TwoBoneIK::SolveConstrained(EditablePose& pose, int root, int middle, int end, Vector3 target,
                                 Vector3 pole, const AnimationIKConstraint& constraint,
                                 float weight, float tolerance, AnimationIKResult& result) {
  constexpr float pi = 3.14159265358979323846f;
  const Vector3 preferred{constraint.preferredDirection[0], constraint.preferredDirection[1],
                          constraint.preferredDirection[2]};
  const Vector3 previous{constraint.previousDirection[0], constraint.previousDirection[1],
                         constraint.previousDirection[2]};
  if (!std::isfinite(weight) || weight < 0.0f || weight > 1.0f || !std::isfinite(tolerance) ||
      tolerance < 0.0f || !std::isfinite(constraint.minBend) ||
      !std::isfinite(constraint.maxBend) || constraint.minBend < 0.0f || constraint.maxBend > pi ||
      constraint.minBend > constraint.maxBend || constraint.hasPreviousDirection > 1 ||
      !Finite(target) || !Finite(pole) || !Finite(preferred) || !Finite(previous))
    return false;

  EditablePose solved = pose;
  AnimationIKResult solvedResult{};
  if (!SolveInternal(solved, root, middle, end, target, pole, tolerance, &constraint,
                     &solvedResult))
    return false;
  EditablePose blended = weight == 1.0f ? std::move(solved) : pose;
  if (weight != 1.0f && !Blend(blended, solved, root, middle, weight)) return false;

  AnimationBoneTransform a{}, b{}, c{};
  if (!blended.Get(root, AnimationBoneSpace::Model, a) ||
      !blended.Get(middle, AnimationBoneSpace::Model, b) ||
      !blended.Get(end, AnimationBoneSpace::Model, c))
    return false;
  const Vector3 upper = Vector3Subtract(Position(b), Position(a));
  const Vector3 lower = Vector3Subtract(Position(c), Position(b));
  const float dot =
      std::clamp(Vector3DotProduct(Vector3Normalize(upper), Vector3Normalize(lower)), -1.0f, 1.0f);
  solvedResult.bendAngle = std::acos(dot);
  solvedResult.endError = Vector3Distance(Position(c), target);
  Vector3 axis = Vector3Subtract(target, Position(a));
  if (Vector3LengthSqr(axis) > 1.0e-12f) {
    axis = Vector3Normalize(axis);
    Vector3 bend = Vector3Subtract(upper, Vector3Scale(axis, Vector3DotProduct(upper, axis)));
    if (Vector3LengthSqr(bend) > 1.0e-12f) {
      bend = Vector3Normalize(bend);
      solvedResult.bendDirection[0] = bend.x;
      solvedResult.bendDirection[1] = bend.y;
      solvedResult.bendDirection[2] = bend.z;
    }
  }
  if (!std::isfinite(solvedResult.bendAngle) || !std::isfinite(solvedResult.endError)) return false;
  pose = std::move(blended);
  result = solvedResult;
  return true;
}

bool TwoBoneIK::Solve(EditablePose& pose, int root, int middle, int end, Vector3 target,
                      Vector3 pole, float tolerance) {
  if (!std::isfinite(tolerance) || tolerance < 0.0f) return false;

  EditablePose candidate = pose;

  if (!SolveInternal(candidate, root, middle, end, target, pole, tolerance, nullptr, nullptr))
    return false;

  pose = std::move(candidate);
  return true;
}

bool TwoBoneIK::SolveInternal(EditablePose& pose, int root, int middle, int end, Vector3 target,
                              Vector3 pole, float tolerance,
                              const AnimationIKConstraint* constraint, AnimationIKResult* result) {
  constexpr float epsilon = 1.0e-6f;

  if (!pose.IsSampled() || root < 0 || middle < 0 || end < 0 || pose.Parent(middle) != root ||
      pose.Parent(end) != middle || !Finite(target) || !Finite(pole) || !std::isfinite(tolerance) ||
      tolerance < 0.0f)
    return false;

  AnimationBoneTransform rootTransform{};
  AnimationBoneTransform middleTransform{};
  AnimationBoneTransform endTransform{};

  if (!pose.Get(root, AnimationBoneSpace::Model, rootTransform) ||
      !pose.Get(middle, AnimationBoneSpace::Model, middleTransform) ||
      !pose.Get(end, AnimationBoneSpace::Model, endTransform))
    return false;

  const Vector3 a = Position(rootTransform);
  const Vector3 b = Position(middleTransform);
  const Vector3 c = Position(endTransform);

  const float upperLength = Vector3Distance(a, b);
  const float lowerLength = Vector3Distance(b, c);

  if (!std::isfinite(upperLength) || !std::isfinite(lowerLength) || upperLength <= epsilon ||
      lowerLength <= epsilon)
    return false;

  Vector3 direction = Vector3Subtract(target, a);
  const float targetDistance = Vector3Length(direction);

  if (targetDistance <= epsilon) {
    direction = Vector3Subtract(c, a);

    if (Vector3LengthSqr(direction) <= epsilon * epsilon) direction = Vector3Subtract(b, a);

    if (Vector3LengthSqr(direction) <= epsilon * epsilon) return false;
  }

  direction = Vector3Normalize(direction);

  const float minReach = std::abs(upperLength - lowerLength);
  const float maxReach = upperLength + lowerLength;
  float constrainedMin = minReach;
  float constrainedMax = maxReach;
  if (constraint) {
    const auto radiusAt = [upperLength, lowerLength](float angle) {
      return std::sqrt(std::max(0.0f, upperLength * upperLength + lowerLength * lowerLength +
                                          2.0f * upperLength * lowerLength * std::cos(angle)));
    };
    constrainedMin = radiusAt(constraint->maxBend);
    constrainedMax = radiusAt(constraint->minBend);
    if (result) {
      result->reachable = targetDistance >= constrainedMin - tolerance &&
                          targetDistance <= constrainedMax + tolerance;
    }
  }

  // Keep the chain slightly away from singular configurations.
  const float reachMargin = std::min(epsilon, maxReach * 0.0001f);

  const float lowerBound = std::max(constrainedMin, minReach + reachMargin);
  const float upperBound = std::min(constrainedMax, maxReach - reachMargin);

  // A point boundary at a singular angle uses the nearest numerically stable radius.
  const float lower = lowerBound > upperBound ? std::clamp(constrainedMin, minReach + reachMargin,
                                                           maxReach - reachMargin)
                                              : lowerBound;
  const float upper = lowerBound > upperBound ? lower : upperBound;

  const float reach = std::clamp(targetDistance, lower, upper);

  // Construct a bend direction perpendicular to the target.
  const auto project = [direction](Vector3 value) {
    return Vector3Subtract(value, Vector3Scale(direction, Vector3DotProduct(value, direction)));
  };
  Vector3 poleDirection{};
  if (constraint && constraint->hasPreviousDirection) {
    poleDirection = project({constraint->previousDirection[0], constraint->previousDirection[1],
                             constraint->previousDirection[2]});
    if (Vector3LengthSqr(poleDirection) > epsilon * epsilon && result)
      result->usedPreviousDirection = 1;
  }
  if (Vector3LengthSqr(poleDirection) <= epsilon * epsilon && constraint)
    poleDirection = project({constraint->preferredDirection[0], constraint->preferredDirection[1],
                             constraint->preferredDirection[2]});
  if (Vector3LengthSqr(poleDirection) <= epsilon * epsilon)
    poleDirection = project(Vector3Subtract(pole, a));

  if (Vector3LengthSqr(poleDirection) <= epsilon * epsilon) {
    poleDirection = project(Vector3Subtract(b, a));
  }

  if (Vector3LengthSqr(poleDirection) <= epsilon * epsilon) {
    const Vector3 reference = std::abs(direction.x) <= std::abs(direction.y) &&
                                      std::abs(direction.x) <= std::abs(direction.z)
                                  ? Vector3{1.0f, 0.0f, 0.0f}
                              : std::abs(direction.y) <= std::abs(direction.z)
                                  ? Vector3{0.0f, 1.0f, 0.0f}
                                  : Vector3{0.0f, 0.0f, 1.0f};
    poleDirection = Vector3CrossProduct(direction, reference);
    if (result) result->usedFallbackDirection = 1;
  }

  if (Vector3LengthSqr(poleDirection) <= epsilon * epsilon) return false;

  poleDirection = Vector3Normalize(poleDirection);
  if (result) {
    result->bendDirection[0] = poleDirection.x;
    result->bendDirection[1] = poleDirection.y;
    result->bendDirection[2] = poleDirection.z;
  }

  // Law of cosines.
  const float along =
      (upperLength * upperLength + reach * reach - lowerLength * lowerLength) / (2.0f * reach);

  const float height = std::sqrt(std::max(0.0f, upperLength * upperLength - along * along));

  const Vector3 desiredMiddle = Vector3Add(
      a, Vector3Add(Vector3Scale(direction, along), Vector3Scale(poleDirection, height)));

  const Vector3 desiredEnd = Vector3Add(a, Vector3Scale(direction, reach));

  // Rotate the root to aim the upper segment.
  Quaternion rootDelta{};

  if (!RotationBetween(Vector3Subtract(b, a), Vector3Subtract(desiredMiddle, a), rootDelta))
    return false;

  SetRotation(rootTransform, QuaternionMultiply(rootDelta, Rotation(rootTransform)));

  if (!pose.Set(root, AnimationBoneSpace::Model, rootTransform)) return false;

  // Read updated positions after rotating the root.
  if (!pose.Get(middle, AnimationBoneSpace::Model, middleTransform) ||
      !pose.Get(end, AnimationBoneSpace::Model, endTransform))
    return false;

  // Rotate the middle joint toward the desired end position.
  Quaternion middleDelta{};

  if (!RotationBetween(Vector3Subtract(Position(endTransform), Position(middleTransform)),
                       Vector3Subtract(desiredEnd, Position(middleTransform)), middleDelta))
    return false;

  SetRotation(middleTransform, QuaternionMultiply(middleDelta, Rotation(middleTransform)));

  if (!pose.Set(middle, AnimationBoneSpace::Model, middleTransform)) return false;

  // Validate the final end-effector position.
  AnimationBoneTransform finalEnd{};

  if (!pose.Get(end, AnimationBoneSpace::Model, finalEnd)) return false;

  const Vector3 actualEnd = Position(finalEnd);
  const float error = Vector3Distance(actualEnd, desiredEnd);

  if (!std::isfinite(error)) return false;

  if (error > tolerance + 1.0e-5f) {
    std::fprintf(stderr,
                 "[TwoBoneIK] Validation failed\n"
                 "  Target:  %.6f %.6f %.6f\n"
                 "  Desired: %.6f %.6f %.6f\n"
                 "  Actual:  %.6f %.6f %.6f\n"
                 "  Error:   %.8f\n"
                 "  Allowed: %.8f\n",
                 target.x, target.y, target.z, desiredEnd.x, desiredEnd.y, desiredEnd.z,
                 actualEnd.x, actualEnd.y, actualEnd.z, error, tolerance + 1.0e-5f);
  }

  return error <= tolerance + 1.0e-5f;
}

bool TwoBoneIK::RotationBetween(Vector3 from, Vector3 to, Quaternion& result) {
  constexpr float epsilon = 1.0e-6f;

  const float fromLength = Vector3Length(from);
  const float toLength = Vector3Length(to);

  if (!std::isfinite(fromLength) || !std::isfinite(toLength) || fromLength <= epsilon ||
      toLength <= epsilon)
    return false;

  from = Vector3Scale(from, 1.0f / fromLength);
  to = Vector3Scale(to, 1.0f / toLength);

  const float dot = std::clamp(Vector3DotProduct(from, to), -1.0f, 1.0f);

  if (dot >= 1.0f - epsilon) {
    result = QuaternionIdentity();
    return true;
  }

  Vector3 axis = Vector3CrossProduct(from, to);

  // Handle opposite directions.
  if (Vector3LengthSqr(axis) <= epsilon * epsilon) {
    const Vector3 reference =
        std::abs(from.x) < 0.9f ? Vector3{1.0f, 0.0f, 0.0f} : Vector3{0.0f, 1.0f, 0.0f};

    axis = Vector3CrossProduct(from, reference);
  }

  if (Vector3LengthSqr(axis) <= epsilon * epsilon) return false;

  axis = Vector3Normalize(axis);

  result = QuaternionFromAxisAngle(axis, std::acos(dot));

  return true;
}

}  // namespace Levye
