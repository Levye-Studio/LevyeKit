#pragma once

#include <raylib.h>

#include <array>
#include <cstdint>

namespace Levye {

inline constexpr int MaxSupportContacts = 64;

/** @brief Caller-owned world-space support; inactive and zero-weight contacts are ignored. */
struct SupportContact {
  std::uint64_t id = 0;
  std::uint64_t limbId = 0;
  Vector3 positionWorld{};
  Vector3 normalWorld{0, 1, 0};
  float confidence = 1;
  bool active = false;
};

enum class SupportRegionKind : std::uint32_t { None, Point, Segment, Polygon };
enum class SupportNormalStatus : std::uint32_t { None, Valid, Conflicting };

/** @brief Projected convex support region; margin is positive inside and negative outside.
 * Point/segment stability uses a caller tolerance and has no fabricated area. Hull vertices
 * are counterclockwise when viewed along `up`, starting at lexicographically smallest point. */
struct SupportRegion {
  SupportRegionKind kind = SupportRegionKind::None;
  int activeCount = 0;
  int hullCount = 0;
  std::array<Vector3, MaxSupportContacts> hullWorld{};
  Vector3 centroidWorld{};
  Vector3 projectedReferenceWorld{};
  Vector3 correctionWorld{};
  Vector3 correctionDirectionWorld{};
  float nearestBoundaryDistance = 0;
  float stabilityMargin = 0;
  bool stable = false;
};

/** @brief Radius limits from a caller-owned limb attachment to its world contact. */
struct StabilizationReach {
  std::uint64_t contactId = 0;
  Vector3 attachmentWorld{};
  Vector3 contactWorld{};
  float minimum = 0;
  float maximum = 0;
  bool active = false;
};

struct HeightSuggestion {
  float offsetAlongUp = 0;
  float heightAboveSupport = 0;
  int violatedReachCount = 0;
  bool feasible = true;
  bool hasSupport = false;
};

/** @brief All lengths are world units; angles are radians. */
struct StabilizationConfig {
  Vector3 up{0, 1, 0};
  float pointTolerance = 0.08f;
  float nominalHeight = 0.85f;
  float minimumHeight = 0.55f;
  float maximumHeight = 1.1f;
  float horizontalResponse = 8;
  float heightResponse = 6;
  float orientationResponse = 6;
  float maximumCorrection = 0.16f;
  float maximumTilt = 0.35f;
  float orientationWeight = 0.5f;
  float strength = 1;
};

struct StabilizationResult {
  SupportRegion support{};
  HeightSuggestion height{};
  Vector3 positionOffsetWorld{};
  Vector3 suggestedCorrectionWorld{};
  Quaternion orientationAdjustment{0, 0, 0, 1};
  Quaternion bodyOrientationWorld{0, 0, 0, 1};
  Vector3 movementIntentWorld{};
  SupportNormalStatus normalStatus = SupportNormalStatus::None;
  float tiltRadians = 0;
  bool reachFeasible = true;
};

/** @brief Stateless world-space support analysis; invalid active inputs leave output unchanged. */
[[nodiscard]] bool AnalyzeSupport(const SupportContact* contacts, int count, Vector3 up,
                                  Vector3 referenceWorld, float pointTolerance,
                                  SupportRegion& output) noexcept;

/** @brief Suggests a bounded root height. Infeasible reach sets are reported and receive
 * a deterministic best-effort height; contact anchors are never changed. */
[[nodiscard]] bool SuggestBodyHeight(Vector3 rootWorld, const SupportRegion& support, Vector3 up,
                                     float preferredHeight, float minimumHeight,
                                     float maximumHeight, const StabilizationReach* reaches,
                                     int reachCount, HeightSuggestion& output) noexcept;

/** @brief Stateful kinematic suggestions; caller retains movement intent and applies results. */
class BodyStabilizer {
 public:
  void Reset() noexcept;
  void SetPaused(bool paused) noexcept { m_Paused = paused; }
  void SetEnabled(bool enabled) noexcept;
  [[nodiscard]] bool IsPaused() const noexcept { return m_Paused; }
  [[nodiscard]] bool IsEnabled() const noexcept { return m_Enabled; }

  [[nodiscard]] bool Update(float deltaTime, Vector3 currentRootWorld, Vector3 desiredRootWorld,
                            Quaternion headingWorld, const SupportContact* contacts,
                            int contactCount, const StabilizationReach* reaches, int reachCount,
                            const StabilizationConfig& config,
                            StabilizationResult& output) noexcept;

 private:
  Vector3 m_HorizontalOffset{};
  float m_HeightOffset = 0;
  Quaternion m_Tilt{0, 0, 0, 1};
  bool m_Enabled = true;
  bool m_Paused = false;
};
}  // namespace Levye
