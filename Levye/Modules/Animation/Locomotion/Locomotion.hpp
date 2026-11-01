#pragma once

#include <raylib.h>
#include <raymath.h>

#include <Levye/Modules/Animation/Contact/ContactTargets.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace Levye {

enum class GaitPhase : std::uint32_t { Stance, Swing };

/** @brief Endpoint-preserving swing shape. Values of 1 and 2 reproduce the original curve. */
struct SwingEasing {
  float landing = 1.0f;
  float lift = 2.0f;
};

/** @brief One generic limb; nominal contact is in root-local space. */
struct GaitLimb {
  std::uint64_t id = 0;
  std::uint64_t chainId = 0;
  float phaseOffset = 0;
  float dutyFactor = 0.6f;
  Vector3 nominalLocal{};
  float maxStrideReach = 1;
  float stepHeight = 0.25f;
  bool enabled = true;
  SwingEasing easing{};
};

/** @brief Phase and events for one limb during the most recent advance. */
struct GaitLimbState {
  float phase = 0;
  GaitPhase state = GaitPhase::Stance;
  float progress = 0;
  std::uint64_t stepStarts = 0;
  std::uint64_t stepEnds = 0;
};

/** @brief Caller-owned scheduler. Global phase is in cycles [0, 1); frequency is cycles/second. */
class GaitScheduler {
 public:
  [[nodiscard]] bool Configure(const std::vector<GaitLimb>& limbs) {
    for (std::size_t i = 0; i < limbs.size(); ++i) {
      const auto& limb = limbs[i];
      if (!limb.id || !limb.chainId || !std::isfinite(limb.phaseOffset) || limb.phaseOffset < 0 ||
          limb.phaseOffset >= 1 || !std::isfinite(limb.dutyFactor) || limb.dutyFactor <= 0 ||
          limb.dutyFactor >= 1 || !std::isfinite(limb.nominalLocal.x) ||
          !std::isfinite(limb.nominalLocal.y) || !std::isfinite(limb.nominalLocal.z) ||
          !std::isfinite(limb.maxStrideReach) || limb.maxStrideReach <= 0 ||
          !std::isfinite(limb.stepHeight) || limb.stepHeight < 0)
        return false;
      for (std::size_t j = 0; j < i; ++j)
        if (limbs[j].id == limb.id || limbs[j].chainId == limb.chainId) return false;
    }
    m_Limbs = limbs;
    m_States.resize(limbs.size());
    Reset();
    return true;
  }

  void Reset() noexcept {
    m_Cycles = 0;
    m_Paused = false;
    Refresh(0, 0);
  }

  void SetPaused(bool paused) noexcept { m_Paused = paused; }
  [[nodiscard]] bool IsPaused() const noexcept { return m_Paused; }
  [[nodiscard]] float GlobalPhase() const noexcept {
    return static_cast<float>(m_Cycles - std::floor(m_Cycles));
  }
  [[nodiscard]] const std::vector<GaitLimb>& Limbs() const noexcept { return m_Limbs; }
  [[nodiscard]] const std::vector<GaitLimbState>& States() const noexcept { return m_States; }

  /** @brief Advances phase; event counts include every boundary crossed by a large delta. */
  [[nodiscard]] bool Advance(float deltaTime, float frequency) noexcept {
    if (!std::isfinite(deltaTime) || deltaTime < 0 || !std::isfinite(frequency) || frequency < 0)
      return false;
    const double before = m_Cycles;
    const double step = m_Paused ? 0 : static_cast<double>(deltaTime) * frequency;
    if (!std::isfinite(step) || before + step > 1.0e12) return false;
    m_Cycles += step;
    Refresh(before, m_Cycles);
    return true;
  }

 private:
  void Refresh(double before, double after) noexcept {
    for (std::size_t i = 0; i < m_Limbs.size(); ++i) {
      const auto& limb = m_Limbs[i];
      auto& output = m_States[i];
      const double cycle = after + limb.phaseOffset;
      const double phase = cycle - std::floor(cycle);
      output.phase = static_cast<float>(phase);
      output.state = phase < limb.dutyFactor ? GaitPhase::Stance : GaitPhase::Swing;
      output.progress = static_cast<float>(output.state == GaitPhase::Stance
                                               ? phase / limb.dutyFactor
                                               : (phase - limb.dutyFactor) / (1 - limb.dutyFactor));
      output.stepStarts =
          limb.enabled
              ? static_cast<std::uint64_t>(std::floor(after + limb.phaseOffset - limb.dutyFactor) -
                                           std::floor(before + limb.phaseOffset - limb.dutyFactor))
              : 0;
      output.stepEnds = limb.enabled
                            ? static_cast<std::uint64_t>(std::floor(after + limb.phaseOffset) -
                                                         std::floor(before + limb.phaseOffset))
                            : 0;
    }
  }

  std::vector<GaitLimb> m_Limbs;
  std::vector<GaitLimbState> m_States;
  double m_Cycles = 0;
  bool m_Paused = false;
};

/** @brief Example offsets; callers may change every duty factor, offset, or limb setting. */
[[nodiscard]] inline std::vector<GaitLimb> GaitPreset(int limbCount) {
  if (limbCount != 2 && limbCount != 4 && limbCount != 6) return {};
  std::vector<GaitLimb> limbs;
  limbs.reserve(static_cast<std::size_t>(limbCount));
  for (int i = 0; i < limbCount; ++i) {
    const int pair = i / 2;
    const bool firstSide = i % 2 == 0;
    const float offset = limbCount == 2   ? (firstSide ? 0.0f : 0.5f)
                         : limbCount == 4 ? ((pair + (firstSide ? 0 : 1)) % 2) * 0.5f
                                          : ((pair + (firstSide ? 0 : 1)) % 2) * 0.5f;
    limbs.push_back({static_cast<std::uint64_t>(i + 1),
                     static_cast<std::uint64_t>(i + 1),
                     offset,
                     limbCount == 2   ? 0.62f
                     : limbCount == 4 ? 0.64f
                                      : 0.67f,
                     {firstSide ? -0.72f : 0.72f, 0, (pair - (limbCount / 2 - 1) * 0.5f) * 0.8f},
                     0.42f,
                     0.20f,
                     true});
  }
  return limbs;
}

struct SwingSample {
  Vector3 position{};
  Vector3 tangent{};
};

/** @brief Smooth world-space swing. Lift and tangent vanish at both endpoints. */
[[nodiscard]] inline bool SampleSwing(Vector3 start, Vector3 landing, Vector3 up, float progress,
                                      float height, SwingEasing easing,
                                      SwingSample& output) noexcept {
  const auto finite = [](Vector3 v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
  };
  if (!finite(start) || !finite(landing) || !finite(up) || !std::isfinite(Vector3LengthSqr(up)) ||
      Vector3LengthSqr(up) < 1.0e-12f || !std::isfinite(progress) || progress < 0 || progress > 1 ||
      !std::isfinite(height) || height < 0 || !std::isfinite(easing.landing) ||
      easing.landing < 1 || easing.landing > 4 || !std::isfinite(easing.lift) || easing.lift < 2 ||
      easing.lift > 6)
    return false;
  const float smooth = progress * progress * (3 - 2 * progress);
  const float left = std::pow(smooth, easing.landing);
  const float right = std::pow(1 - smooth, easing.landing);
  const float travel = left / (left + right);
  const float slope = easing.landing * std::pow(smooth, easing.landing - 1) *
                      std::pow(1 - smooth, easing.landing - 1) / ((left + right) * (left + right)) *
                      6 * progress * (1 - progress);
  const float liftBase = 4 * progress * (1 - progress);
  const float lift = std::pow(liftBase, easing.lift);
  const float liftSlope =
      easing.lift * std::pow(liftBase, easing.lift - 1) * 4 * (1 - 2 * progress);
  const Vector3 direction = Vector3Subtract(landing, start);
  const Vector3 normalizedUp = Vector3Normalize(up);
  SwingSample sample{};
  sample.position = Vector3Add(Vector3Add(start, Vector3Scale(direction, travel)),
                               Vector3Scale(normalizedUp, height * lift));
  sample.tangent =
      Vector3Add(Vector3Scale(direction, slope), Vector3Scale(normalizedUp, height * liftSlope));
  if (!finite(sample.position) || !finite(sample.tangent)) return false;
  output = sample;
  return true;
}

[[nodiscard]] inline bool SampleSwing(Vector3 start, Vector3 landing, Vector3 up, float progress,
                                      float height, SwingSample& output) noexcept {
  return SampleSwing(start, landing, up, progress, height, {}, output);
}

/** @brief Shortest-path end rotation between captured liftoff and landing orientations.
 * The zero-slope endpoints make stance/swing orientation continuous. */
[[nodiscard]] inline bool SampleSwingOrientation(Quaternion start, Quaternion landing,
                                                 float progress, Quaternion& output) noexcept {
  const auto finite = [](Quaternion q) {
    return std::isfinite(q.x) && std::isfinite(q.y) && std::isfinite(q.z) && std::isfinite(q.w) &&
           std::isfinite(QuaternionLength(q)) && QuaternionLength(q) > 1.0e-8f;
  };
  if (!finite(start) || !finite(landing) || !std::isfinite(progress) || progress < 0 ||
      progress > 1)
    return false;
  start = QuaternionNormalize(start);
  landing = QuaternionNormalize(landing);
  const float t = progress * progress * (3 - 2 * progress);
  output = QuaternionNormalize(QuaternionSlerp(start, landing, t));
  return true;
}

enum class LandingStatus : std::uint32_t { Accepted, Clamped, NoSurface, Invalid };

struct LandingPlan {
  Vector3 world{};
  Vector3 normal{};
  LandingStatus status = LandingStatus::Invalid;
};

struct LimbReach {
  Vector3 targetWorld{};
  float rootDistance = 0;
  float minimum = 0;
  float maximum = 0;
  bool reachable = false;
};

/** @brief Limits a world target to a two-segment chain's practical radial range. */
[[nodiscard]] inline bool EvaluateLimbReach(Vector3 root, Vector3 target, Vector3 fallbackDirection,
                                            float upperLength, float lowerLength, float margin,
                                            LimbReach& output) noexcept {
  const auto finite = [](Vector3 v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
  };
  if (!finite(root) || !finite(target) || !finite(fallbackDirection) ||
      !std::isfinite(Vector3LengthSqr(fallbackDirection)) ||
      Vector3LengthSqr(fallbackDirection) < 1.0e-12f || !std::isfinite(upperLength) ||
      !std::isfinite(lowerLength) || upperLength <= 0 || lowerLength <= 0 ||
      !std::isfinite(margin) || margin < 0)
    return false;
  const float minimum = std::abs(upperLength - lowerLength) + margin;
  const float maximum = upperLength + lowerLength - margin;
  if (minimum > maximum) return false;
  Vector3 direction = Vector3Subtract(target, root);
  const float distance = Vector3Length(direction);
  if (!std::isfinite(distance)) return false;
  direction = distance > 1.0e-6f ? Vector3Scale(direction, 1.0f / distance)
                                 : Vector3Normalize(fallbackDirection);
  LimbReach result{};
  result.rootDistance = distance;
  result.minimum = minimum;
  result.maximum = maximum;
  result.reachable = distance >= minimum && distance <= maximum;
  result.targetWorld =
      Vector3Add(root, Vector3Scale(direction, std::clamp(distance, minimum, maximum)));
  output = result;
  return true;
}

/** @brief Plans a world landing from caller root motion and a caller-supplied terrain hit. */
[[nodiscard]] inline bool PlanLanding(Matrix rootWorld, Vector3 nominalLocal, Vector3 velocityWorld,
                                      float stepDuration, float maxReach, Vector3 up,
                                      const ContactSurfaceHit& hit, LandingPlan& output) noexcept {
  const auto finite = [](Vector3 v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
  };
  if (!IsFiniteContactMatrix(rootWorld) || !finite(nominalLocal) || !finite(velocityWorld) ||
      !finite(up) || !std::isfinite(Vector3LengthSqr(up)) || Vector3LengthSqr(up) < 1.0e-12f ||
      !std::isfinite(stepDuration) || stepDuration < 0 || !std::isfinite(maxReach) || maxReach <= 0)
    return false;
  const bool invalidSurface = hit.valid && (!finite(hit.position) || !finite(hit.normal) ||
                                            !std::isfinite(Vector3LengthSqr(hit.normal)) ||
                                            Vector3LengthSqr(hit.normal) < 1.0e-12f);
  const bool validSurface = hit.valid && !invalidSurface;
  const Vector3 nominal = Vector3Transform(nominalLocal, rootWorld);
  const Vector3 predicted = Vector3Add(nominal, Vector3Scale(velocityWorld, stepDuration));
  const Vector3 candidate = validSurface ? hit.position : predicted;
  Vector3 displacement = Vector3Subtract(candidate, nominal);
  const float distance = Vector3Length(displacement);
  if (!finite(candidate) || !std::isfinite(distance)) return false;
  LandingPlan plan{};
  plan.status = invalidSurface ? LandingStatus::Invalid
                : validSurface ? LandingStatus::Accepted
                               : LandingStatus::NoSurface;
  if (distance > maxReach) {
    displacement = Vector3Scale(displacement, maxReach / distance);
    if (validSurface) plan.status = LandingStatus::Clamped;
  }
  plan.world = Vector3Add(nominal, displacement);
  plan.normal = validSurface ? Vector3Normalize(hit.normal) : Vector3Normalize(up);
  output = plan;
  return true;
}

/** @brief One caller-owned limb target; stance uses ContactTargets, swing uses SampleSwing. */
struct LocomotionTarget {
  ContactController contact{};
  Vector3 targetWorld{};
  Vector3 swingStart{};
  LandingPlan landing{};
  float swingHeight = 0;
  SwingEasing swingEasing{};
  bool initialized = false;

  void Reset() noexcept { *this = {}; }

  [[nodiscard]] bool Update(const GaitLimb& limb, const GaitLimbState& phase,
                            Vector3 animationEndWorld, const LandingPlan& planned,
                            const ContactSurfaceHit& terrainHit, ContactConfig config, Vector3 up,
                            float deltaTime) noexcept {
    const auto finite = [](Vector3 v) {
      return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    };
    if (!finite(animationEndWorld) || !finite(planned.world) || !finite(up) ||
        !std::isfinite(Vector3LengthSqr(up)) || Vector3LengthSqr(up) < 1.0e-12f ||
        !std::isfinite(deltaTime) || deltaTime < 0 || !std::isfinite(phase.progress) ||
        phase.progress < 0 || phase.progress > 1 || !std::isfinite(limb.stepHeight) ||
        limb.stepHeight < 0 || !std::isfinite(limb.easing.landing) || limb.easing.landing < 1 ||
        limb.easing.landing > 4 || !std::isfinite(limb.easing.lift) || limb.easing.lift < 2 ||
        limb.easing.lift > 6 ||
        ((planned.status == LandingStatus::Accepted || planned.status == LandingStatus::Clamped) &&
         (!finite(planned.normal) || Vector3LengthSqr(planned.normal) < 1.0e-12f)))
      return false;
    LocomotionTarget next = *this;
    if (!limb.enabled) {
      next.contact.Reset();
      next.targetWorld = animationEndWorld;
      next.initialized = true;
      *this = next;
      return true;
    }
    if (!next.initialized) {
      next.targetWorld = animationEndWorld;
      next.swingStart = animationEndWorld;
      next.landing = planned;
      next.swingHeight = limb.stepHeight;
      next.swingEasing = limb.easing;
      next.initialized = true;
    }
    if (phase.stepStarts) {
      next.swingStart = next.targetWorld;
      next.landing = planned;
      next.swingHeight = limb.stepHeight;
      next.swingEasing = limb.easing;
      next.contact.Release();
    }
    if (phase.state == GaitPhase::Swing) {
      ContactSurfaceHit miss{};
      config.releaseOnMiss = true;
      if (!next.contact.Update(animationEndWorld, miss, config, deltaTime)) return false;
      SwingSample sample{};
      if (!SampleSwing(next.swingStart, next.landing.world, up, phase.progress, next.swingHeight,
                       next.swingEasing, sample))
        return false;
      next.targetWorld = sample.position;
    } else {
      if (phase.stepEnds) next.contact.Reset();
      ContactSurfaceHit hit = terrainHit;
      const LandingPlan& selected = phase.stepEnds ? next.landing : planned;
      hit.valid = hit.valid && (selected.status == LandingStatus::Accepted ||
                                selected.status == LandingStatus::Clamped);
      if (hit.valid) {
        hit.position = selected.world;
        hit.normal = selected.normal;
      }
      const Vector3 base = phase.stepEnds ? next.targetWorld : animationEndWorld;
      if (!next.contact.Update(base, hit, config, deltaTime)) return false;
      if (next.contact.state != ContactState::Inactive)
        next.targetWorld = next.contact.targetWorld;
      else
        next.targetWorld = base;
    }
    *this = next;
    return true;
  }
};

}  // namespace Levye
