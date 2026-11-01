#pragma once

#include <raylib.h>
#include <raymath.h>

#include <Levye/Modules/Animation/AnimationAPI.hpp>
#include <Levye/Modules/Animation/TargetSmoothing.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace Levye {

[[nodiscard]] inline bool IsFiniteContactMatrix(Matrix m) noexcept {
  const std::array values = {m.m0, m.m1, m.m2,  m.m3,  m.m4,  m.m5,  m.m6,  m.m7,
                             m.m8, m.m9, m.m10, m.m11, m.m12, m.m13, m.m14, m.m15};
  for (float value : values)
    if (!std::isfinite(value)) return false;
  return true;
}

enum class ContactState : std::uint32_t { Inactive, Acquiring, Locked, Releasing };
enum class ContactHitStatus : std::uint32_t {
  NoHit,
  Accepted,
  InvalidNormal,
  TooFar,
  TooSteep,
  LowConfidence,
  InvalidInput
};

/** @brief Caller-supplied world-space surface query; orientation does not drive positional IK. */
struct ContactSurfaceHit {
  std::uint64_t contactId = 0;
  std::uint64_t chainId = 0;
  std::uint64_t surfaceId = 0;
  Vector3 position{};
  Vector3 normal{0, 1, 0};
  Quaternion orientation{0, 0, 0, 1};
  float confidence = 1.0f;
  bool hasOrientation = false;
  bool valid = false;
};

/** @brief Optional current world transform of a moving surface; use a rigid transform
 * when orientation tracking is enabled. */
struct ContactSurfaceTransform {
  std::uint64_t surfaceId = 0;
  Matrix worldFromSurface = MatrixIdentity();
  bool valid = false;
};

/** @brief Contact filtering and response settings; all positions and vectors are world-space. */
struct ContactConfig {
  Vector3 up{0, 1, 0};
  float maxDistance = 2.0f;
  float maxSlopeRadians = 1.04719755f;
  float minConfidence = 0.0f;
  float offsetAlongNormal = 0.0f;
  float acquireRate = 8.0f;
  float releaseRate = 8.0f;
  float lockTolerance = 0.02f;
  bool releaseOnMiss = false;
  bool followSurface = false;
  bool reacquireOnSurfaceChange = false;
};

/** @brief Caller-owned state. A locked world anchor never follows the character root. */
struct ContactController {
  ContactState state = ContactState::Inactive;
  ContactHitStatus hitStatus = ContactHitStatus::NoHit;
  std::uint64_t contactId = 0;
  std::uint64_t chainId = 0;
  std::uint64_t surfaceId = 0;
  Vector3 targetWorld{};
  Vector3 anchorWorld{};
  Vector3 normalWorld{0, 1, 0};
  Quaternion orientationWorld{0, 0, 0, 1};
  Quaternion orientationSurfaceLocal{0, 0, 0, 1};
  Vector3 normalSurfaceLocal{0, 1, 0};
  Vector3 anchorSurfaceLocal{};
  float weight = 0.0f;
  bool hasOrientation = false;
  bool boundToSurface = false;

  void Reset() noexcept { *this = {}; }
  void Release() noexcept {
    if (state == ContactState::Acquiring || state == ContactState::Locked)
      state = ContactState::Releasing;
  }

  /** @brief Advances one deterministic contact; invalid configuration leaves state unchanged. */
  [[nodiscard]] bool Update(Vector3 animationTargetWorld, const ContactSurfaceHit& hit,
                            const ContactConfig& config, float deltaTime,
                            const ContactSurfaceTransform* surface = nullptr) noexcept {
    const auto finite = [](Vector3 v) {
      return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    };
    constexpr float pi = 3.14159265358979323846f;
    const float upLength = Vector3LengthSqr(config.up);
    if (!finite(animationTargetWorld) || !finite(config.up) || !std::isfinite(upLength) ||
        upLength < 1.0e-12f || !std::isfinite(deltaTime) || deltaTime < 0 ||
        !std::isfinite(config.maxDistance) || config.maxDistance < 0 ||
        !std::isfinite(config.maxSlopeRadians) || config.maxSlopeRadians < 0 ||
        config.maxSlopeRadians > pi || !std::isfinite(config.minConfidence) ||
        config.minConfidence < 0 || config.minConfidence > 1 ||
        !std::isfinite(config.offsetAlongNormal) || !std::isfinite(config.acquireRate) ||
        config.acquireRate < 0 || !std::isfinite(config.releaseRate) || config.releaseRate < 0 ||
        !std::isfinite(config.lockTolerance) || config.lockTolerance < 0)
      return false;

    ContactHitStatus status = ContactHitStatus::NoHit;
    Vector3 contactPoint{};
    Vector3 normal{};
    if (hit.valid) {
      if (!finite(hit.position) || !finite(hit.normal) || !std::isfinite(hit.confidence) ||
          (hit.hasOrientation &&
           (!std::isfinite(hit.orientation.x) || !std::isfinite(hit.orientation.y) ||
            !std::isfinite(hit.orientation.z) || !std::isfinite(hit.orientation.w) ||
            !std::isfinite(
                hit.orientation.x * hit.orientation.x + hit.orientation.y * hit.orientation.y +
                hit.orientation.z * hit.orientation.z + hit.orientation.w * hit.orientation.w) ||
            hit.orientation.x * hit.orientation.x + hit.orientation.y * hit.orientation.y +
                    hit.orientation.z * hit.orientation.z + hit.orientation.w * hit.orientation.w <
                1.0e-12f))) {
        status = ContactHitStatus::InvalidInput;
      } else if (!std::isfinite(Vector3LengthSqr(hit.normal)) ||
                 Vector3LengthSqr(hit.normal) < 1.0e-12f) {
        status = ContactHitStatus::InvalidNormal;
      } else if (hit.confidence < config.minConfidence) {
        status = ContactHitStatus::LowConfidence;
      } else if (Vector3Distance(animationTargetWorld, hit.position) > config.maxDistance) {
        status = ContactHitStatus::TooFar;
      } else {
        normal = Vector3Normalize(hit.normal);
        const float slopeDot = Vector3DotProduct(normal, Vector3Normalize(config.up));
        if (slopeDot < std::cos(config.maxSlopeRadians))
          status = ContactHitStatus::TooSteep;
        else {
          status = ContactHitStatus::Accepted;
          contactPoint = Vector3Add(hit.position, Vector3Scale(normal, config.offsetAlongNormal));
        }
      }
    }

    hitStatus = status;
    const bool accepted = status == ContactHitStatus::Accepted;
    if (state == ContactState::Inactive) {
      targetWorld = animationTargetWorld;
      weight = 0;
      if (accepted) state = ContactState::Acquiring;
    }
    if (state == ContactState::Acquiring) {
      if (!accepted) {
        state = ContactState::Releasing;
      } else {
        contactId = hit.contactId;
        chainId = hit.chainId;
        surfaceId = hit.surfaceId;
        normalWorld = normal;
        orientationWorld =
            hit.hasOrientation ? QuaternionNormalize(hit.orientation) : Quaternion{0, 0, 0, 1};
        hasOrientation = hit.hasOrientation;
        targetWorld = SmoothTarget(targetWorld, contactPoint, deltaTime, config.acquireRate);
        weight = 1.0f - (1.0f - weight) * std::exp(-config.acquireRate * deltaTime);
        if (weight >= 0.99f && Vector3Distance(targetWorld, contactPoint) <= config.lockTolerance) {
          state = ContactState::Locked;
          anchorWorld = contactPoint;
          targetWorld = anchorWorld;
          boundToSurface = false;
          if (config.followSurface && surface && surface->valid &&
              surface->surfaceId == surfaceId && IsFiniteContactMatrix(surface->worldFromSurface) &&
              std::isfinite(MatrixDeterminant(surface->worldFromSurface)) &&
              std::abs(MatrixDeterminant(surface->worldFromSurface)) > 1.0e-8f) {
            anchorSurfaceLocal =
                Vector3Transform(anchorWorld, MatrixInvert(surface->worldFromSurface));
            const Quaternion surfaceRotation =
                QuaternionNormalize(QuaternionFromMatrix(surface->worldFromSurface));
            orientationSurfaceLocal = QuaternionNormalize(
                QuaternionMultiply(QuaternionInvert(surfaceRotation), orientationWorld));
            normalSurfaceLocal =
                Vector3RotateByQuaternion(normalWorld, QuaternionInvert(surfaceRotation));
            boundToSurface = true;
          }
        }
      }
    }
    if (state == ContactState::Locked) {
      if (config.reacquireOnSurfaceChange && accepted && hit.surfaceId != surfaceId) {
        state = ContactState::Acquiring;
      } else if (config.releaseOnMiss && !accepted) {
        state = ContactState::Releasing;
      } else {
        if (boundToSurface && surface && surface->valid && surface->surfaceId == surfaceId &&
            IsFiniteContactMatrix(surface->worldFromSurface) &&
            std::isfinite(MatrixDeterminant(surface->worldFromSurface)) &&
            std::abs(MatrixDeterminant(surface->worldFromSurface)) > 1.0e-8f) {
          anchorWorld = Vector3Transform(anchorSurfaceLocal, surface->worldFromSurface);
          const Quaternion surfaceRotation =
              QuaternionNormalize(QuaternionFromMatrix(surface->worldFromSurface));
          orientationWorld =
              QuaternionNormalize(QuaternionMultiply(surfaceRotation, orientationSurfaceLocal));
          normalWorld =
              Vector3Normalize(Vector3RotateByQuaternion(normalSurfaceLocal, surfaceRotation));
        }
        targetWorld = anchorWorld;
        weight = 1.0f;
      }
    }
    if (state == ContactState::Releasing) {
      targetWorld = SmoothTarget(targetWorld, animationTargetWorld, deltaTime, config.releaseRate);
      weight *= std::exp(-config.releaseRate * deltaTime);
      if (weight <= 0.01f) {
        state = ContactState::Inactive;
        weight = 0;
        targetWorld = animationTargetWorld;
        boundToSurface = false;
      }
    }
    return true;
  }
};

/** @brief Converts a world contact to a model-space positional IK request. */
[[nodiscard]] inline bool ContactToIKRequest(const ContactController& contact,
                                             const AnimationIKChainRequest& templateRequest,
                                             Matrix modelTransform, Vector3 drawPosition,
                                             AnimationIKChainRequest& output) noexcept {
  if (!std::isfinite(contact.weight) || !std::isfinite(templateRequest.weight) ||
      !std::isfinite(drawPosition.x) || !std::isfinite(drawPosition.y) ||
      !std::isfinite(drawPosition.z))
    return false;
  AnimationIKChainRequest candidate = templateRequest;
  candidate.enabled =
      templateRequest.enabled && contact.state != ContactState::Inactive && contact.weight > 0 ? 1U
                                                                                               : 0U;
  candidate.weight = std::clamp(templateRequest.weight * contact.weight, 0.0f, 1.0f);
  if (!candidate.enabled) {
    output = candidate;
    return true;
  }
  if (contact.chainId != templateRequest.id || !IsFiniteContactMatrix(modelTransform) ||
      !std::isfinite(MatrixDeterminant(modelTransform)) ||
      std::abs(MatrixDeterminant(modelTransform)) < 1.0e-8f)
    return false;
  const Vector3 model = Vector3Transform(Vector3Subtract(contact.targetWorld, drawPosition),
                                         MatrixInvert(modelTransform));
  if (!std::isfinite(model.x) || !std::isfinite(model.y) || !std::isfinite(model.z)) return false;
  candidate.target[0] = model.x;
  candidate.target[1] = model.y;
  candidate.target[2] = model.z;
  output = candidate;
  return true;
}

}  // namespace Levye
