#include "EndEffectorOrientation.hpp"

#include <raymath.h>

#include <algorithm>
#include <cmath>

namespace Levye {
namespace {
constexpr float Pi = 3.14159265358979323846f;
constexpr float Epsilon = 1.0e-8f;

bool Finite(Vector3 v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
bool Finite(Quaternion q) {
  return std::isfinite(q.x) && std::isfinite(q.y) && std::isfinite(q.z) && std::isfinite(q.w) &&
         std::isfinite(QuaternionLength(q)) && QuaternionLength(q) > Epsilon;
}
Quaternion Read(const float q[4]) { return {q[0], q[1], q[2], q[3]}; }
float Dot(Quaternion a, Quaternion b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
Quaternion Negate(Quaternion q) { return {-q.x, -q.y, -q.z, -q.w}; }
float Angle(Quaternion a, Quaternion b) {
  return 2.0f * std::acos(std::clamp(std::abs(Dot(a, b)), 0.0f, 1.0f));
}
Vector3 Project(Vector3 v, Vector3 normal) {
  return Vector3Subtract(v, Vector3Scale(normal, Vector3DotProduct(v, normal)));
}
bool Valid(const EditablePose& pose, const AnimationOrientationRequest& r) {
  const Vector3 axis{r.twistAxis[0], r.twistAxis[1], r.twistAxis[2]};
  const Vector3 normal{r.surfaceNormal[0], r.surfaceNormal[1], r.surfaceNormal[2]};
  const Vector3 forward{r.referenceForward[0], r.referenceForward[1], r.referenceForward[2]};
  const Vector3 up{r.localContactUp[0], r.localContactUp[1], r.localContactUp[2]};
  const Vector3 localForward{r.localContactForward[0], r.localContactForward[1],
                             r.localContactForward[2]};
  return r.bone >= 0 && r.bone < pose.BoneCount() &&
         (r.space == AnimationBoneSpace::Local || r.space == AnimationBoneSpace::Model) &&
         r.enabled <= 1 && r.constrained <= 1 && r.hasSurface <= 1 && r.hasPrevious <= 1 &&
         r.worldSpace <= 1 &&
         (!r.worldSpace ||
          (r.space == AnimationBoneSpace::Model && Finite(Read(r.worldFromModel)))) &&
         Finite(Read(r.desired)) && (!r.hasPrevious || Finite(Read(r.previous))) &&
         (!r.hasSurface ||
          (Finite(normal) && Vector3LengthSqr(normal) > Epsilon && Finite(forward) && Finite(up) &&
           Vector3LengthSqr(up) > Epsilon && Finite(localForward) &&
           Vector3LengthSqr(Project(localForward, Vector3Normalize(up))) > Epsilon)) &&
         Finite(Read(r.reference)) && Finite(axis) && Vector3LengthSqr(axis) > Epsilon &&
         std::isfinite(r.weight) && r.weight >= 0 && r.weight <= 1 && std::isfinite(r.maxAngle) &&
         r.maxAngle >= 0 && r.maxAngle <= Pi && std::isfinite(r.maxTwist) && r.maxTwist >= 0 &&
         r.maxTwist <= Pi;
}
bool Ancestor(const EditablePose& pose, int ancestor, int bone) {
  for (int current = bone; current >= 0; current = pose.Parent(current))
    if (current == ancestor) return true;
  return false;
}
Vector3 Fallback(Vector3 normal) {
  Vector3 axis{1, 0, 0};
  if (std::abs(normal.y) < std::abs(normal.x)) axis = {0, 1, 0};
  if (std::abs(normal.z) < std::abs(Vector3DotProduct(normal, axis))) axis = {0, 0, 1};
  return Vector3Normalize(Project(axis, normal));
}
}  // namespace

bool EndEffectorOrientation::Solve(EditablePose& pose, const AnimationOrientationRequest& request,
                                   AnimationOrientationResult& result) {
  result = {};
  result.id = request.id;
  result.status =
      !request.enabled ? AnimationOrientationStatus::Disabled : AnimationOrientationStatus::Invalid;
  if (!request.enabled) return true;
  if (!pose.IsSampled() || !Valid(pose, request)) return false;
  AnimationBoneTransform local{}, parent{}, model{};
  if (!pose.Get(request.bone, AnimationBoneSpace::Local, local) ||
      !pose.Get(request.bone, AnimationBoneSpace::Model, model)) {
    result.status = AnimationOrientationStatus::Failed;
    return false;
  }
  const int parentIndex = pose.Parent(request.bone);
  Quaternion parentRotation{0, 0, 0, 1};
  if (parentIndex >= 0) {
    if (!pose.Get(parentIndex, AnimationBoneSpace::Model, parent)) {
      result.status = AnimationOrientationStatus::Failed;
      return false;
    }
    parentRotation = QuaternionNormalize(Read(parent.rotation));
  }
  const Quaternion modelFromWorld =
      request.worldSpace ? QuaternionInvert(QuaternionNormalize(Read(request.worldFromModel)))
                         : QuaternionIdentity();
  Quaternion desired = QuaternionNormalize(Read(request.desired));
  if (request.worldSpace)
    desired = QuaternionNormalize(QuaternionMultiply(modelFromWorld, desired));
  if (request.hasSurface) {
    Quaternion previous = Read(request.previous);
    Vector3 normal{request.surfaceNormal[0], request.surfaceNormal[1], request.surfaceNormal[2]};
    Vector3 forward{request.referenceForward[0], request.referenceForward[1],
                    request.referenceForward[2]};
    if (request.worldSpace) {
      normal = Vector3RotateByQuaternion(normal, modelFromWorld);
      forward = Vector3RotateByQuaternion(forward, modelFromWorld);
      if (request.hasPrevious)
        previous = QuaternionNormalize(QuaternionMultiply(modelFromWorld, previous));
    }
    if (!SurfaceOrientation(
            normal, forward,
            {request.localContactUp[0], request.localContactUp[1], request.localContactUp[2]},
            {request.localContactForward[0], request.localContactForward[1],
             request.localContactForward[2]},
            desired, request.hasPrevious ? &previous : nullptr)) {
      result.status = AnimationOrientationStatus::Failed;
      return false;
    }
  }
  const Quaternion target = desired;
  if (request.constrained) {
    Quaternion reference = QuaternionNormalize(Read(request.reference));
    if (request.worldSpace)
      reference = QuaternionNormalize(QuaternionMultiply(modelFromWorld, reference));
    Quaternion delta =
        QuaternionNormalize(QuaternionMultiply(QuaternionInvert(reference), desired));
    const Vector3 axis =
        Vector3Normalize({request.twistAxis[0], request.twistAxis[1], request.twistAxis[2]});
    const Vector3 imaginary{delta.x, delta.y, delta.z};
    const Vector3 projected = Vector3Scale(axis, Vector3DotProduct(imaginary, axis));
    Quaternion twist{projected.x, projected.y, projected.z, delta.w};
    if (QuaternionLength(twist) <= Epsilon) twist = {0, 0, 0, 1};
    twist = QuaternionNormalize(twist);
    const float signedTwist =
        2.0f * std::atan2(Vector3DotProduct({twist.x, twist.y, twist.z}, axis), twist.w);
    const float wrappedTwist = std::remainder(signedTwist, 2.0f * Pi);
    const Quaternion limitedTwist = QuaternionFromAxisAngle(
        axis, std::clamp(wrappedTwist, -request.maxTwist, request.maxTwist));
    const Quaternion swing =
        QuaternionNormalize(QuaternionMultiply(delta, QuaternionInvert(twist)));
    delta = QuaternionNormalize(QuaternionMultiply(swing, limitedTwist));
    if (delta.w < 0) delta = Negate(delta);
    const float angle = Angle({0, 0, 0, 1}, delta);
    if (angle > request.maxAngle)
      delta = QuaternionSlerp({0, 0, 0, 1}, delta, request.maxAngle / angle);
    desired = QuaternionNormalize(QuaternionMultiply(reference, delta));
    result.constrained = Angle(desired, target) > 1.0e-4f;
  }
  const Quaternion desiredLocal =
      request.space == AnimationBoneSpace::Model
          ? QuaternionNormalize(QuaternionMultiply(QuaternionInvert(parentRotation), desired))
          : desired;
  Quaternion current = QuaternionNormalize(Read(local.rotation));
  Quaternion endpoint = desiredLocal;
  if (Dot(current, endpoint) < 0) endpoint = Negate(endpoint);
  const Quaternion blended =
      QuaternionNormalize(QuaternionSlerp(current, endpoint, request.weight));
  local.rotation[0] = blended.x;
  local.rotation[1] = blended.y;
  local.rotation[2] = blended.z;
  local.rotation[3] = blended.w;
  if (!pose.Set(request.bone, AnimationBoneSpace::Local, local)) {
    result.status = AnimationOrientationStatus::Failed;
    return false;
  }
  const Quaternion applied = request.space == AnimationBoneSpace::Model
                                 ? QuaternionNormalize(QuaternionMultiply(parentRotation, blended))
                                 : blended;
  result.angularError = Angle(applied, target);
  result.status = AnimationOrientationStatus::Solved;
  return true;
}

bool EndEffectorOrientation::SolveBatch(EditablePose& pose,
                                        const AnimationOrientationRequest* requests, int count,
                                        AnimationOrientationResult* results) {
  if (!pose.IsSampled() || count < 0 || (count > 0 && (!requests || !results))) return false;
  for (int i = 0; i < count; ++i) {
    results[i] = {};
    results[i].id = requests[i].id;
    results[i].status = !requests[i].enabled       ? AnimationOrientationStatus::Disabled
                        : Valid(pose, requests[i]) ? AnimationOrientationStatus::Failed
                                                   : AnimationOrientationStatus::Invalid;
  }
  for (int i = 0; i < count; ++i) {
    for (int j = i + 1; j < count; ++j) {
      if (!requests[i].enabled || !requests[j].enabled) continue;
      if (requests[i].id == requests[j].id) {
        results[i].status = AnimationOrientationStatus::Invalid;
        results[j].status = AnimationOrientationStatus::Invalid;
      }
      if (results[i].status == AnimationOrientationStatus::Invalid ||
          results[j].status == AnimationOrientationStatus::Invalid)
        continue;
      if (Ancestor(pose, requests[i].bone, requests[j].bone) ||
          Ancestor(pose, requests[j].bone, requests[i].bone)) {
        results[i].status = AnimationOrientationStatus::Conflict;
        results[j].status = AnimationOrientationStatus::Conflict;
      }
    }
  }
  std::uint64_t last = 0;
  bool haveLast = false;
  for (int processed = 0; processed < count; ++processed) {
    int index = -1;
    for (int i = 0; i < count; ++i)
      if (results[i].status == AnimationOrientationStatus::Failed &&
          (!haveLast || requests[i].id > last) &&
          (index < 0 || requests[i].id < requests[index].id))
        index = i;
    if (index < 0) break;
    last = requests[index].id;
    haveLast = true;
    (void)Solve(pose, requests[index], results[index]);
  }
  return true;
}

bool EndEffectorOrientation::SurfaceOrientation(Vector3 normal, Vector3 forward, Vector3 localUp,
                                                Vector3 localForward, Quaternion& output,
                                                const Quaternion* previous) {
  if (!Finite(normal) || !Finite(forward) || !Finite(localUp) || !Finite(localForward) ||
      Vector3LengthSqr(normal) <= Epsilon || Vector3LengthSqr(localUp) <= Epsilon ||
      Vector3LengthSqr(Project(localForward, Vector3Normalize(localUp))) <= Epsilon ||
      (previous && !Finite(*previous)))
    return false;
  normal = Vector3Normalize(normal);
  localUp = Vector3Normalize(localUp);
  const Quaternion upRotation = Vector3DotProduct(localUp, normal) < -0.9999f
                                    ? QuaternionFromAxisAngle(Fallback(localUp), Pi)
                                    : QuaternionFromVector3ToVector3(localUp, normal);
  Vector3 currentForward = Project(Vector3RotateByQuaternion(localForward, upRotation), normal);
  if (Vector3LengthSqr(currentForward) <= Epsilon) return false;
  currentForward = Vector3Normalize(currentForward);
  Vector3 targetForward = Project(forward, normal);
  const float forwardLengthSquared = Vector3LengthSqr(forward);
  const float nearParallelThreshold = std::max(Epsilon, forwardLengthSquared * 1.0e-4f);
  if (Vector3LengthSqr(targetForward) <= nearParallelThreshold && previous)
    targetForward = Project(Vector3RotateByQuaternion(localForward, *previous), normal);
  if (Vector3LengthSqr(targetForward) <= nearParallelThreshold) targetForward = Fallback(normal);
  targetForward = Vector3Normalize(targetForward);
  const float angle =
      std::atan2(Vector3DotProduct(normal, Vector3CrossProduct(currentForward, targetForward)),
                 Vector3DotProduct(currentForward, targetForward));
  Quaternion candidate =
      QuaternionNormalize(QuaternionMultiply(QuaternionFromAxisAngle(normal, angle), upRotation));
  if (previous && Dot(candidate, *previous) < 0) candidate = Negate(candidate);
  output = candidate;
  return true;
}
}  // namespace Levye
