#include "BodyStabilization.hpp"

#include <raymath.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace Levye {
namespace {
constexpr float Pi = 3.14159265358979323846f;
constexpr float Epsilon = 1.0e-6f;

bool Finite(Vector3 v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
bool Finite(Quaternion q) {
  return std::isfinite(q.x) && std::isfinite(q.y) && std::isfinite(q.z) && std::isfinite(q.w) &&
         std::isfinite(QuaternionLength(q)) && QuaternionLength(q) > Epsilon;
}
Vector3 Tangent(Vector3 value, Vector3 up) {
  return Vector3Subtract(value, Vector3Scale(up, Vector3DotProduct(value, up)));
}
Vector3 Basis(Vector3 up) {
  Vector3 axis{1, 0, 0};
  if (std::abs(up.y) < std::abs(up.x)) axis = {0, 1, 0};
  if (std::abs(up.z) < std::abs(Vector3DotProduct(axis, up))) axis = {0, 0, 1};
  return Vector3Normalize(Tangent(axis, up));
}
struct Point {
  double x = 0;
  double y = 0;
  std::uint64_t id = 0;
};
double Cross(Point a, Point b, Point c) {
  return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}
double DistanceSquared(Point a, Point b) {
  const double x = a.x - b.x;
  const double y = a.y - b.y;
  return x * x + y * y;
}
Point Closest(Point p, Point a, Point b) {
  const double x = b.x - a.x;
  const double y = b.y - a.y;
  const double lengthSquared = x * x + y * y;
  if (lengthSquared <= 1.0e-20) return a;
  const double t = std::clamp(((p.x - a.x) * x + (p.y - a.y) * y) / lengthSquared, 0.0, 1.0);
  return {a.x + t * x, a.y + t * y, 0};
}
float Alpha(float response, float dt) { return 1.0f - std::exp(-response * dt); }
}  // namespace

bool AnalyzeSupport(const SupportContact* contacts, int count, Vector3 up, Vector3 referenceWorld,
                    float pointTolerance, SupportRegion& output) noexcept {
  if (count < 0 || count > MaxSupportContacts || (count && !contacts) || !Finite(up) ||
      Vector3LengthSqr(up) < Epsilon || !Finite(referenceWorld) || !std::isfinite(pointTolerance) ||
      pointTolerance < 0)
    return false;
  up = Vector3Normalize(up);
  const Vector3 u = Basis(up);
  const Vector3 v = Vector3CrossProduct(up, u);
  std::array<Point, MaxSupportContacts> points{};
  Vector3 centroid{};
  float totalWeight = 0;
  int active = 0;
  for (int i = 0; i < count; ++i) {
    const auto& contact = contacts[i];
    if (!contact.active) continue;
    if (!Finite(contact.positionWorld) || !Finite(contact.normalWorld) ||
        Vector3LengthSqr(contact.normalWorld) < Epsilon || !std::isfinite(contact.confidence) ||
        contact.confidence < 0 || contact.confidence > 1)
      return false;
    if (contact.confidence == 0) continue;
    points[static_cast<std::size_t>(active)] = {Vector3DotProduct(contact.positionWorld, u),
                                                Vector3DotProduct(contact.positionWorld, v),
                                                contact.id};
    centroid = Vector3Add(centroid, Vector3Scale(contact.positionWorld, contact.confidence));
    totalWeight += contact.confidence;
    ++active;
  }
  SupportRegion result{};
  result.activeCount = active;
  result.projectedReferenceWorld = referenceWorld;
  if (!active) {
    output = result;
    return true;
  }
  centroid = Vector3Scale(centroid, 1.0f / totalWeight);
  result.centroidWorld = centroid;
  result.projectedReferenceWorld = Vector3Subtract(
      referenceWorld,
      Vector3Scale(up, Vector3DotProduct(Vector3Subtract(referenceWorld, centroid), up)));
  std::sort(points.begin(), points.begin() + active, [](Point a, Point b) {
    if (a.x != b.x) return a.x < b.x;
    if (a.y != b.y) return a.y < b.y;
    return a.id < b.id;
  });
  int unique = 0;
  for (int i = 0; i < active; ++i) {
    if (unique && DistanceSquared(points[i], points[unique - 1]) < 1.0e-12) continue;
    points[unique++] = points[i];
  }
  std::array<Point, 2 * MaxSupportContacts> hull{};
  int size = 0;
  for (int i = 0; i < unique; ++i) {
    while (size >= 2 && Cross(hull[size - 2], hull[size - 1], points[i]) <= 0) --size;
    hull[size++] = points[i];
  }
  const int lower = size;
  for (int i = unique - 2; i >= 0; --i) {
    while (size > lower && Cross(hull[size - 2], hull[size - 1], points[i]) <= 0) --size;
    hull[size++] = points[i];
  }
  if (size > 1) --size;
  result.hullCount = size;
  result.kind = size == 1   ? SupportRegionKind::Point
                : size == 2 ? SupportRegionKind::Segment
                            : SupportRegionKind::Polygon;
  const float planeHeight = Vector3DotProduct(centroid, up);
  for (int i = 0; i < size; ++i)
    result.hullWorld[i] = Vector3Add(Vector3Add(Vector3Scale(u, static_cast<float>(hull[i].x)),
                                                Vector3Scale(v, static_cast<float>(hull[i].y))),
                                     Vector3Scale(up, planeHeight));
  const Point reference{Vector3DotProduct(result.projectedReferenceWorld, u),
                        Vector3DotProduct(result.projectedReferenceWorld, v), 0};
  Point nearest = hull[0];
  double nearestSquared = std::numeric_limits<double>::infinity();
  bool inside = size >= 3;
  const int edges = size == 1 ? 1 : size == 2 ? 1 : size;
  for (int i = 0; i < edges; ++i) {
    const Point a = hull[i];
    const Point b = hull[size == 1 ? 0 : (i + 1) % size];
    const Point candidate = Closest(reference, a, b);
    const double distance = DistanceSquared(reference, candidate);
    if (distance < nearestSquared) {
      nearestSquared = distance;
      nearest = candidate;
    }
    if (size >= 3 && Cross(a, b, reference) < -1.0e-10) inside = false;
  }
  const float boundaryDistance = static_cast<float>(std::sqrt(nearestSquared));
  result.nearestBoundaryDistance = boundaryDistance;
  result.stabilityMargin = size >= 3 ? (inside ? boundaryDistance : -boundaryDistance)
                                     : pointTolerance - boundaryDistance;
  result.stable = size >= 3 ? inside : boundaryDistance <= pointTolerance;
  const Point correction =
      inside ? Point{Vector3DotProduct(centroid, u), Vector3DotProduct(centroid, v), 0} : nearest;
  result.correctionWorld =
      Vector3Add(Vector3Scale(u, static_cast<float>(correction.x - reference.x)),
                 Vector3Scale(v, static_cast<float>(correction.y - reference.y)));
  if (Vector3LengthSqr(result.correctionWorld) > Epsilon)
    result.correctionDirectionWorld = Vector3Normalize(result.correctionWorld);
  output = result;
  return true;
}

bool SuggestBodyHeight(Vector3 rootWorld, const SupportRegion& support, Vector3 up,
                       float preferredHeight, float minimumHeight, float maximumHeight,
                       const StabilizationReach* reaches, int reachCount,
                       HeightSuggestion& output) noexcept {
  if (!Finite(rootWorld) || !Finite(up) || Vector3LengthSqr(up) < Epsilon ||
      !std::isfinite(preferredHeight) || !std::isfinite(minimumHeight) ||
      !std::isfinite(maximumHeight) || minimumHeight < 0 || minimumHeight > maximumHeight ||
      reachCount < 0 || reachCount > MaxSupportContacts || (reachCount && !reaches))
    return false;
  up = Vector3Normalize(up);
  HeightSuggestion result{};
  result.hasSupport = support.kind != SupportRegionKind::None;
  for (int i = 0; i < reachCount; ++i) {
    const auto& reach = reaches[i];
    if (reach.active && (!Finite(reach.attachmentWorld) || !Finite(reach.contactWorld) ||
                         !std::isfinite(reach.minimum) || !std::isfinite(reach.maximum) ||
                         reach.minimum < 0 || reach.minimum > reach.maximum))
      return false;
  }
  if (!result.hasSupport) {
    output = result;
    return true;
  }
  if (!Finite(support.centroidWorld)) return false;
  const float currentHeight =
      Vector3DotProduct(Vector3Subtract(rootWorld, support.centroidWorld), up);
  std::array<float, 4 + 4 * MaxSupportContacts> candidates{};
  int candidateCount = 0;
  const auto add = [&](float height) {
    candidates[static_cast<std::size_t>(candidateCount++)] =
        std::clamp(height, minimumHeight, maximumHeight);
  };
  add(preferredHeight);
  add(minimumHeight);
  add(maximumHeight);
  add(currentHeight);
  for (int i = 0; i < reachCount; ++i) {
    const auto& reach = reaches[i];
    if (!reach.active) continue;
    if (!Finite(reach.attachmentWorld) || !Finite(reach.contactWorld) ||
        !std::isfinite(reach.minimum) || !std::isfinite(reach.maximum) || reach.minimum < 0 ||
        reach.minimum > reach.maximum)
      return false;
    const Vector3 delta = Vector3Subtract(reach.attachmentWorld, reach.contactWorld);
    const float vertical = Vector3DotProduct(delta, up);
    const float lateralSquared = Vector3LengthSqr(Tangent(delta, up));
    const float center = currentHeight - vertical;
    for (float radius : {reach.minimum, reach.maximum}) {
      if (radius * radius < lateralSquared) continue;
      const float half = std::sqrt(std::max(0.0f, radius * radius - lateralSquared));
      add(center - half);
      add(center + half);
    }
  }
  float bestLoss = std::numeric_limits<float>::infinity();
  float bestPreference = std::numeric_limits<float>::infinity();
  float bestHeight = candidates[0];
  int bestViolations = 0;
  for (int i = 0; i < candidateCount; ++i) {
    const float height = candidates[i];
    float loss = 0;
    int violations = 0;
    for (int j = 0; j < reachCount; ++j) {
      const auto& reach = reaches[j];
      if (!reach.active) continue;
      const Vector3 shifted =
          Vector3Add(reach.attachmentWorld, Vector3Scale(up, height - currentHeight));
      const float distance = Vector3Distance(shifted, reach.contactWorld);
      const float error = distance < reach.minimum   ? reach.minimum - distance
                          : distance > reach.maximum ? distance - reach.maximum
                                                     : 0;
      if (error > 0.0001f) ++violations;
      loss += error * error;
    }
    const float preference = std::abs(height - preferredHeight);
    if (loss < bestLoss - 1.0e-7f ||
        (std::abs(loss - bestLoss) <= 1.0e-7f &&
         (preference < bestPreference - 1.0e-7f ||
          (std::abs(preference - bestPreference) <= 1.0e-7f && height < bestHeight)))) {
      bestLoss = loss;
      bestPreference = preference;
      bestHeight = height;
      bestViolations = violations;
    }
  }
  result.offsetAlongUp = bestHeight - currentHeight;
  result.heightAboveSupport = bestHeight;
  result.violatedReachCount = bestViolations;
  result.feasible = bestViolations == 0;
  output = result;
  return true;
}

void BodyStabilizer::Reset() noexcept {
  m_HorizontalOffset = {};
  m_HeightOffset = 0;
  m_Tilt = QuaternionIdentity();
  m_Enabled = true;
  m_Paused = false;
}

void BodyStabilizer::SetEnabled(bool enabled) noexcept {
  if (m_Enabled == enabled) return;
  m_Enabled = enabled;
  m_HorizontalOffset = {};
  m_HeightOffset = 0;
  m_Tilt = QuaternionIdentity();
}

bool BodyStabilizer::Update(float deltaTime, Vector3 currentRootWorld, Vector3 desiredRootWorld,
                            Quaternion headingWorld, const SupportContact* contacts,
                            int contactCount, const StabilizationReach* reaches, int reachCount,
                            const StabilizationConfig& config,
                            StabilizationResult& output) noexcept {
  if (!std::isfinite(deltaTime) || deltaTime < 0 || !Finite(currentRootWorld) ||
      !Finite(desiredRootWorld) || !Finite(headingWorld) || !Finite(config.up) ||
      Vector3LengthSqr(config.up) < Epsilon || !std::isfinite(config.pointTolerance) ||
      config.pointTolerance < 0 || !std::isfinite(config.nominalHeight) ||
      !std::isfinite(config.minimumHeight) || !std::isfinite(config.maximumHeight) ||
      !std::isfinite(config.horizontalResponse) || config.horizontalResponse < 0 ||
      !std::isfinite(config.heightResponse) || config.heightResponse < 0 ||
      !std::isfinite(config.orientationResponse) || config.orientationResponse < 0 ||
      !std::isfinite(config.maximumCorrection) || config.maximumCorrection < 0 ||
      !std::isfinite(config.maximumTilt) || config.maximumTilt < 0 || config.maximumTilt > Pi ||
      !std::isfinite(config.orientationWeight) || config.orientationWeight < 0 ||
      config.orientationWeight > 1 || !std::isfinite(config.strength) || config.strength < 0 ||
      config.strength > 1)
    return false;
  const Vector3 up = Vector3Normalize(config.up);
  SupportRegion region{};
  if (!AnalyzeSupport(contacts, contactCount, up, desiredRootWorld, config.pointTolerance, region))
    return false;
  StabilizationResult result{};
  result.support = region;
  result.movementIntentWorld = Vector3Subtract(desiredRootWorld, currentRootWorld);
  Vector3 targetHorizontal{};
  if (region.kind != SupportRegionKind::None) {
    targetHorizontal = Vector3Scale(region.correctionWorld, config.strength);
    const float length = Vector3Length(targetHorizontal);
    if (length > config.maximumCorrection)
      targetHorizontal = Vector3Scale(targetHorizontal, config.maximumCorrection / length);
  }
  result.suggestedCorrectionWorld = targetHorizontal;
  Vector3 horizontal = m_Enabled && !m_Paused
                           ? Vector3Lerp(m_HorizontalOffset, targetHorizontal,
                                         Alpha(config.horizontalResponse, deltaTime))
                           : m_HorizontalOffset;
  const float horizontalLength = Vector3Length(horizontal);
  if (horizontalLength > config.maximumCorrection)
    horizontal = Vector3Scale(horizontal, config.maximumCorrection / horizontalLength);
  const float currentHeight =
      region.kind == SupportRegionKind::None
          ? 0
          : Vector3DotProduct(Vector3Subtract(desiredRootWorld, region.centroidWorld), up) +
                m_HeightOffset;
  const float preferred = currentHeight + Alpha(config.heightResponse, deltaTime) *
                                              (config.nominalHeight - currentHeight) *
                                              config.strength;
  HeightSuggestion height{};
  if (!SuggestBodyHeight(desiredRootWorld, region, up, preferred, config.minimumHeight,
                         config.maximumHeight, reaches, reachCount, height))
    return false;
  result.height = height;
  result.reachFeasible = height.feasible;
  float heightOffset = region.kind == SupportRegionKind::None
                           ? m_HeightOffset * (1.0f - Alpha(config.heightResponse, deltaTime))
                           : height.offsetAlongUp;
  if (m_Paused) {
    targetHorizontal = m_HorizontalOffset;
    heightOffset = m_HeightOffset;
  }
  Vector3 normalSum{};
  float weightSum = 0;
  for (int i = 0; i < contactCount; ++i) {
    if (!contacts[i].active || contacts[i].confidence <= 0) continue;
    normalSum = Vector3Add(
        normalSum, Vector3Scale(Vector3Normalize(contacts[i].normalWorld), contacts[i].confidence));
    weightSum += contacts[i].confidence;
  }
  Quaternion targetTilt = QuaternionIdentity();
  if (weightSum > 0) {
    const float coherence = Vector3Length(normalSum) / weightSum;
    if (coherence < 0.5f || Vector3DotProduct(Vector3Normalize(normalSum), up) <= 0.05f) {
      result.normalStatus = SupportNormalStatus::Conflicting;
    } else {
      result.normalStatus = SupportNormalStatus::Valid;
      const Vector3 normal = Vector3Normalize(normalSum);
      const float angle = std::acos(std::clamp(Vector3DotProduct(up, normal), -1.0f, 1.0f));
      const Quaternion full = QuaternionFromVector3ToVector3(up, normal);
      targetTilt =
          QuaternionSlerp(QuaternionIdentity(), full,
                          (angle > Epsilon ? std::min(1.0f, config.maximumTilt / angle) : 1.0f) *
                              config.orientationWeight * config.strength);
    }
  }
  Quaternion tilt = m_Enabled && !m_Paused
                        ? QuaternionNormalize(QuaternionSlerp(
                              m_Tilt, targetTilt, Alpha(config.orientationResponse, deltaTime)))
                        : m_Tilt;
  const auto reachesWith = [&](Vector3 shift, Quaternion rotation) {
    for (int i = 0; i < reachCount; ++i) {
      const auto& reach = reaches[i];
      if (!reach.active) continue;
      const Vector3 relative = Vector3Subtract(reach.attachmentWorld, desiredRootWorld);
      const Vector3 transformed = Vector3Add(
          Vector3Add(desiredRootWorld, Vector3Add(shift, Vector3Scale(up, heightOffset))),
          Vector3RotateByQuaternion(relative, rotation));
      const float distance = Vector3Distance(transformed, reach.contactWorld);
      if (!std::isfinite(distance) || distance < reach.minimum - 0.001f ||
          distance > reach.maximum + 0.001f)
        return false;
    }
    return true;
  };
  if (!height.feasible) {
    horizontal = {};
    tilt = QuaternionIdentity();
  } else if (!reachesWith(horizontal, tilt)) {
    if (reachesWith({}, QuaternionIdentity())) {
      float low = 0;
      float high = 1;
      for (int step = 0; step < 18; ++step) {
        const float middle = 0.5f * (low + high);
        if (reachesWith(Vector3Scale(horizontal, middle),
                        QuaternionSlerp(QuaternionIdentity(), tilt, middle)))
          low = middle;
        else
          high = middle;
      }
      horizontal = Vector3Scale(horizontal, low);
      tilt = QuaternionNormalize(QuaternionSlerp(QuaternionIdentity(), tilt, low));
    } else {
      result.reachFeasible = false;
      horizontal = {};
      tilt = QuaternionIdentity();
    }
  }
  if (config.strength == 0) {
    m_HorizontalOffset = {};
    m_HeightOffset = 0;
    m_Tilt = QuaternionIdentity();
  }
  if (!m_Enabled || config.strength == 0) {
    result.positionOffsetWorld = {};
    result.orientationAdjustment = QuaternionIdentity();
    result.tiltRadians = 0;
  } else {
    result.positionOffsetWorld = Vector3Add(horizontal, Vector3Scale(up, heightOffset));
    result.orientationAdjustment = tilt;
    result.tiltRadians = 2.0f * std::acos(std::clamp(std::abs(tilt.w), 0.0f, 1.0f));
  }
  result.bodyOrientationWorld = QuaternionNormalize(
      QuaternionMultiply(result.orientationAdjustment, QuaternionNormalize(headingWorld)));
  if (!m_Paused && m_Enabled && config.strength > 0) {
    m_HorizontalOffset = horizontal;
    m_HeightOffset = heightOffset;
    m_Tilt = tilt;
  }
  output = result;
  return true;
}
}  // namespace Levye
