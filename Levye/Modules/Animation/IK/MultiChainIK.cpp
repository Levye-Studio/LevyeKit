#include "MultiChainIK.hpp"

#include <algorithm>
#include <cmath>

#include "TwoBoneIK.hpp"

namespace Levye {
namespace {
bool Finite(const float value[3]) {
  return std::isfinite(value[0]) && std::isfinite(value[1]) && std::isfinite(value[2]);
}

bool Valid(const EditablePose& pose, const AnimationIKChainRequest& request) {
  constexpr float pi = 3.14159265358979323846f;
  const auto& c = request.constraint;
  return request.root >= 0 && request.middle >= 0 && request.end >= 0 &&
         request.root < pose.BoneCount() && request.middle < pose.BoneCount() &&
         request.end < pose.BoneCount() && pose.Parent(request.middle) == request.root &&
         pose.Parent(request.end) == request.middle && Finite(request.target) &&
         Finite(request.pole) && std::isfinite(request.weight) && request.weight >= 0.0f &&
         request.weight <= 1.0f && std::isfinite(request.tolerance) && request.tolerance >= 0.0f &&
         request.constrained <= 1 && request.enabled <= 1 &&
         (!request.constrained ||
          (std::isfinite(c.minBend) && std::isfinite(c.maxBend) && c.minBend >= 0.0f &&
           c.minBend <= c.maxBend && c.maxBend <= pi)) &&
         Finite(c.preferredDirection) && Finite(c.previousDirection) && c.hasPreviousDirection <= 1;
}

bool AncestorOrSelf(const EditablePose& pose, int ancestor, int bone) {
  for (int current = bone; current >= 0; current = pose.Parent(current))
    if (current == ancestor) return true;
  return false;
}

bool Affects(const EditablePose& pose, const AnimationIKChainRequest& a,
             const AnimationIKChainRequest& b) {
  for (int controlled : {a.root, a.middle})
    for (int referenced : {b.root, b.middle, b.end})
      if (AncestorOrSelf(pose, controlled, referenced)) return true;
  return false;
}
}  // namespace

bool MultiChainIK::Solve(EditablePose& pose, const AnimationIKChainRequest* requests, int count,
                         AnimationIKChainResult* results) {
  if (!pose.IsSampled() || count < 0 || (count > 0 && (!requests || !results))) return false;
  for (int i = 0; i < count; ++i) {
    results[i] = {};
    results[i].id = requests[i].id;
    results[i].status = !requests[i].enabled       ? AnimationIKChainStatus::Disabled
                        : Valid(pose, requests[i]) ? AnimationIKChainStatus::Failed
                                                   : AnimationIKChainStatus::Invalid;
  }
  for (int i = 0; i < count; ++i) {
    for (int j = i + 1; j < count; ++j) {
      if (!requests[i].enabled || !requests[j].enabled) continue;
      if (requests[i].id == requests[j].id) {
        results[i].status = AnimationIKChainStatus::Invalid;
        results[j].status = AnimationIKChainStatus::Invalid;
      }
    }
  }
  for (int i = 0; i < count; ++i) {
    for (int j = i + 1; j < count; ++j) {
      const auto eligible = [](AnimationIKChainStatus status) {
        return status == AnimationIKChainStatus::Failed ||
               status == AnimationIKChainStatus::Conflict;
      };
      if (!eligible(results[i].status) || !eligible(results[j].status)) continue;
      if (Affects(pose, requests[i], requests[j]) || Affects(pose, requests[j], requests[i])) {
        results[i].status = AnimationIKChainStatus::Conflict;
        results[j].status = AnimationIKChainStatus::Conflict;
      }
    }
  }
  std::uint64_t previousId = 0;
  bool havePreviousId = false;
  for (int processed = 0; processed < count; ++processed) {
    int index = -1;
    for (int candidate = 0; candidate < count; ++candidate) {
      if (results[candidate].status != AnimationIKChainStatus::Failed ||
          (havePreviousId && requests[candidate].id <= previousId))
        continue;
      if (index < 0 || requests[candidate].id < requests[index].id) index = candidate;
    }
    if (index < 0) break;
    previousId = requests[index].id;
    havePreviousId = true;
    const auto& request = requests[index];
    AnimationIKConstraint constraint = request.constraint;
    if (!request.constrained) {
      constraint.minBend = 0.0f;
      constraint.maxBend = 3.14159265358979323846f;
    }
    const Vector3 target{request.target[0], request.target[1], request.target[2]};
    const Vector3 pole{request.pole[0], request.pole[1], request.pole[2]};
    AnimationIKResult ik{};
    if (TwoBoneIK::SolveConstrained(pose, request.root, request.middle, request.end, target, pole,
                                    constraint, request.weight, request.tolerance, ik)) {
      results[index].status = AnimationIKChainStatus::Solved;
      results[index].ik = ik;
    }
  }
  return true;
}
}  // namespace Levye
