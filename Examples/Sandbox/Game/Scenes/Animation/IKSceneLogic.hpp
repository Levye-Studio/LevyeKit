#pragma once

#include <raylib.h>
#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace Sandbox::IKSceneLogic {

struct Bone {
  std::string name;
  int parent = -1;
  Vector3 position{};
};

struct Chain {
  int root = -1;
  int middle = -1;
  int end = -1;
  [[nodiscard]] bool Valid() const noexcept { return root >= 0 && middle >= 0 && end >= 0; }
};

inline float Distance(Vector3 a, Vector3 b) {
  const float x = a.x - b.x;
  const float y = a.y - b.y;
  const float z = a.z - b.z;
  return std::sqrt(x * x + y * y + z * z);
}

/** Finds a nondegenerate consecutive chain, preferring the robot's left arm. */
inline Chain FindChain(const std::vector<Bone>& bones, Chain excluded = {}) {
  Chain best{};
  int bestScore = -1;
  for (int end = 0; end < static_cast<int>(bones.size()); ++end) {
    const int middle = bones[static_cast<std::size_t>(end)].parent;
    if (middle < 0 || middle >= static_cast<int>(bones.size())) continue;
    const int root = bones[static_cast<std::size_t>(middle)].parent;
    if (root < 0 || root >= static_cast<int>(bones.size()) || root == end) continue;
    if (root == excluded.root || root == excluded.middle || root == excluded.end ||
        middle == excluded.root || middle == excluded.middle || middle == excluded.end ||
        end == excluded.root || end == excluded.middle || end == excluded.end)
      continue;
    if (Distance(bones[static_cast<std::size_t>(root)].position,
                 bones[static_cast<std::size_t>(middle)].position) < 0.05f ||
        Distance(bones[static_cast<std::size_t>(middle)].position,
                 bones[static_cast<std::size_t>(end)].position) < 0.05f)
      continue;
    const auto& rootName = bones[static_cast<std::size_t>(root)].name;
    const auto& middleName = bones[static_cast<std::size_t>(middle)].name;
    const auto& endName = bones[static_cast<std::size_t>(end)].name;
    int score = 0;
    if (rootName.find("UpperArm") != std::string::npos &&
        middleName.find("LowerArm") != std::string::npos)
      score += 100;
    if (endName.find("Palm") != std::string::npos || endName.find("Hand") != std::string::npos)
      score += 20;
    if (rootName.find(".L") != std::string::npos) score += 10;
    if (score > bestScore) {
      best = {root, middle, end};
      bestScore = score;
    }
  }
  return best;
}

/** Resets to a small, reachable offset from the sampled end effector. */
inline bool ResetTarget(const Bone& root, const Bone& middle, const Bone& end, Vector3& target,
                        Vector3& pole) {
  const float upper = Distance(root.position, middle.position);
  const float lower = Distance(middle.position, end.position);
  if (!std::isfinite(upper) || !std::isfinite(lower) || upper <= 0.05f || lower <= 0.05f)
    return false;

  const float minimum = std::abs(upper - lower);
  const float maximum = upper + lower;
  const float margin = std::min(0.05f, 0.1f * std::min(upper, lower));
  const float low = minimum + margin;
  const float high = maximum - margin;
  if (low >= high) return false;

  Vector3 direction = Vector3Subtract(end.position, root.position);
  const float currentDistance = Vector3Length(direction);
  if (currentDistance < 1.0e-5f) direction = Vector3Subtract(middle.position, root.position);
  direction = Vector3Normalize(direction);
  const float base = std::clamp(currentDistance, low, high);
  const float offset = std::min(0.1f, 0.15f * std::min(upper, lower));
  const float distance =
      std::clamp(base + (base > (low + high) * 0.5f ? -offset : offset), low, high);
  Vector3 bend = Vector3Subtract(middle.position, root.position);
  bend = Vector3Subtract(bend, Vector3Scale(direction, Vector3DotProduct(bend, direction)));
  if (Vector3LengthSqr(bend) < 1.0e-6f) bend = Vector3CrossProduct(direction, {0.0f, 1.0f, 0.0f});
  if (Vector3LengthSqr(bend) < 1.0e-6f) bend = Vector3CrossProduct(direction, {1.0f, 0.0f, 0.0f});
  if (Vector3LengthSqr(bend) < 1.0e-6f) return false;

  target = Vector3Add(root.position, Vector3Scale(direction, distance));
  pole = Vector3Add(middle.position, Vector3Scale(Vector3Normalize(bend), 0.6f));
  return true;
}

/** Tests the target against both ends of the chain's reachable interval. */
inline bool IsReachable(Vector3 root, Vector3 middle, Vector3 end, Vector3 target) {
  const float upper = Distance(root, middle);
  const float lower = Distance(middle, end);
  const float distance = Distance(root, target);
  return std::isfinite(distance) && distance >= std::abs(upper - lower) - 1.0e-5f &&
         distance <= upper + lower + 1.0e-5f;
}

/** Applies the same model transform and draw translation used by DrawModel(). */
inline Vector3 ToWorld(Vector3 modelPoint, Matrix modelTransform, Vector3 drawPosition = {}) {
  return Vector3Add(Vector3Transform(modelPoint, modelTransform), drawPosition);
}

/** Moves the target in model space using units per second. */
inline void MoveTarget(Vector3& target, float horizontal, float vertical, float depth, float dt) {
  if (!std::isfinite(dt) || dt <= 0.0f) return;
  constexpr float speed = 1.5f;
  target.x += horizontal * speed * dt;
  target.y += vertical * speed * dt;
  target.z += depth * speed * dt;
}

/** Invokes the solver only for an enabled, discovered chain. */
template <class Solver>
bool SolveIfEnabled(bool enabled, Chain chain, Solver&& solve) {
  return enabled && chain.Valid() && solve();
}

}  // namespace Sandbox::IKSceneLogic
