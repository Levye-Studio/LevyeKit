
#pragma once

#include <raylib.h>
#include <raymath.h>

#include <cstdint>
#include <limits>

namespace Levye {

/** @brief Backend selected when creating a physics world. */
enum class PhysicsBackendType : std::uint8_t { Jolt, Bullet, PhysX };

/** @brief Rigid-body motion category. */
enum class PhysicsMotionType : std::uint8_t { Static, Dynamic, Kinematic };

/** @brief Supported body and query shapes. */
enum class PhysicsShapeType : std::uint8_t {
  Box,
  Sphere,
  Capsule,
  ConvexHull,
  TriangleMesh,
  Compound
};

/** @brief Body-pair contact transition emitted by the host. */
enum class PhysicsContactEventType : std::uint8_t { Begin, Persist, End };

/** @brief Generation-checked rigid-body handle, scoped to a world. */
struct PhysicsBodyHandle {
  std::uint32_t index = std::numeric_limits<std::uint32_t>::max();

  std::uint32_t generation = 0;

  [[nodiscard]] constexpr bool IsValid() const {
    return index != std::numeric_limits<std::uint32_t>::max();
  }

  constexpr bool operator==(const PhysicsBodyHandle&) const = default;
};

/** @brief Generation-checked handle for a host-owned physics world. */
struct PhysicsWorldHandle {
  std::uint32_t index = std::numeric_limits<std::uint32_t>::max();

  std::uint32_t generation = 0;

  [[nodiscard]] constexpr bool IsValid() const {
    return index != std::numeric_limits<std::uint32_t>::max();
  }

  constexpr bool operator==(const PhysicsWorldHandle&) const = default;
};

struct PhysicsCompoundChildDesc;

/**
 * @brief Shape data copied synchronously into a host-owned Jolt shape.
 *
 * Hulls use vertices; meshes use vertices and triangle indices (three per
 * triangle); compounds use children. Caller arrays need only remain valid for
 * the duration of CreateBody or the spatial-query call.
 */
struct PhysicsShapeDesc {
  PhysicsShapeType type = PhysicsShapeType::Box;

  Vector3 halfExtents{0.5f, 0.5f, 0.5f};

  float radius = 0.5f;
  float halfHeight = 0.5f;
  const Vector3* vertices = nullptr;
  std::uint32_t vertexCount = 0;
  const std::uint32_t* indices = nullptr;
  std::uint32_t indexCount = 0;
  const PhysicsCompoundChildDesc* children = nullptr;
  std::uint32_t childCount = 0;
};

/** @brief Child shape and local rigid transform in a compound. */
struct PhysicsCompoundChildDesc {
  const PhysicsShapeDesc* shape = nullptr;
  Transform localTransform{{0, 0, 0}, {0, 0, 0, 1}, {1, 1, 1}};
};

/** @brief Rigid-body creation settings. */
struct PhysicsBodyDesc {
  PhysicsMotionType motion = PhysicsMotionType::Dynamic;

  PhysicsShapeDesc shape{};

  Transform transform{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}};

  float mass = 1.0f;
  float friction = 0.5f;
  float restitution = 0.0f;

  std::uint16_t layer = 1;
  /**
   * @brief Collision categories this body accepts.
   *
   * A collision is permitted only when both bodies'
   * layer/mask combinations accept one another.
   */
  uint16_t mask = 0xFFFF;
  bool isSensor = false;
};

/** @brief Physics-world creation settings. */
struct PhysicsWorldDesc {
  PhysicsBackendType backend = PhysicsBackendType::Jolt;

  Vector3 gravity{0.0f, -9.81f, 0.0f};

  std::uint32_t maxBodies = 65536;
  std::uint32_t maxBodyPairs = 16384;
  std::uint32_t maxContactConstraints = 16384;
};

/** @brief Host-owned counters for one physics world, sampled on the update thread. */
struct PhysicsWorldDiagnostics {
  std::uint32_t bodyCount = 0;
  std::uint32_t characterCount = 0;
  std::uint32_t constraintCount = 0;
  std::uint32_t activeContactPairs = 0;
  std::uint32_t pendingContactEvents = 0;
  std::uint64_t completedSteps = 0;
  std::uint64_t rejectedCreations = 0;
  double lastStepMilliseconds = 0.0;
  bool paused = false;
};

/** @brief Closest raycast hit in world space. */
struct PhysicsRaycastHit {
  PhysicsBodyHandle body{};

  Vector3 position{};
  Vector3 normal{};

  float fraction = 0.0f;
};

/**
 * @brief Contact information produced by a physics world.
 *
 * Handles refer to public LevyeKit bodies, never backend-local bodies.
 * End events retain the original body handles and sensor flag even when a
 * body has been destroyed. Destroyed handles identify the old body but are
 * no longer valid for physics operations.
 */
struct PhysicsContactEvent {
  PhysicsContactEventType type = PhysicsContactEventType::Begin;

  PhysicsBodyHandle bodyA{};
  PhysicsBodyHandle bodyB{};

  Vector3 position{};
  Vector3 normal{};

  bool isSensor = false;
};

/**
 * @brief Options controlling which physics bodies a raycast can hit.
 */
struct PhysicsRaycastFilter {
  uint16_t mask = 0xFFFF;
  bool includeSensors = true;
};

/**
 * @brief Controls which bodies participate in spatial queries.
 * A valid ignoredBody must belong to the queried world.
 */
struct PhysicsQueryFilter {
  std::uint16_t mask = 0xFFFF;
  bool includeSensors = true;

  // Optional body to exclude from the query.
  PhysicsBodyHandle ignoredBody{};
};

/**
 * @brief Closest shape-cast hit.
 *
 * position is the world-space point on the hit body's surface. normal points
 * away from that body toward the cast shape. fraction is distance travelled
 * divided by maxDistance; initial penetrations report fraction zero.
 */
struct PhysicsShapeCastHit {
  PhysicsBodyHandle body{};

  Vector3 position{};
  Vector3 normal{};

  // Normalized travel fraction [0, 1].
  float fraction = 0.0f;
};

/** @brief Generation-checked, world-owned virtual character handle. */
struct PhysicsCharacterHandle {
  std::uint32_t index = std::numeric_limits<std::uint32_t>::max();
  std::uint32_t generation = 0;
  [[nodiscard]] constexpr bool IsValid() const {
    return index != std::numeric_limits<std::uint32_t>::max();
  }
  constexpr bool operator==(const PhysicsCharacterHandle&) const = default;
};

/** @brief Generation-checked, world-owned rigid-body constraint handle. */
struct PhysicsConstraintHandle {
  std::uint32_t index = std::numeric_limits<std::uint32_t>::max();
  std::uint32_t generation = 0;
  [[nodiscard]] constexpr bool IsValid() const {
    return index != std::numeric_limits<std::uint32_t>::max();
  }
  constexpr bool operator==(const PhysicsConstraintHandle&) const = default;
};

/** @brief Rigid-body constraint family. */
enum class PhysicsConstraintType : std::uint8_t { Fixed, Point, Hinge, Slider };

/** @brief Motor behavior for a hinge or slider. */
enum class PhysicsMotorMode : std::uint8_t { Off, Velocity, Position };

/**
 * @brief Hinge torque or slider force motor settings.
 *
 * target is radians/s or radians for a hinge, metres/s or metres for a slider.
 * maxForce is torque for a hinge and linear force for a slider.
 */
struct PhysicsMotorDesc {
  PhysicsMotorMode mode = PhysicsMotorMode::Off;
  float target = 0.0f;
  float maxForce = 100.0f;
  float frequency = 2.0f;
  float damping = 1.0f;
};

/**
 * @brief Two-body constraint configured in world space at creation time.
 *
 * Fixed constraints weld the bodies at their current relative transform.
 * Point, hinge, and slider use anchor. Hinge and slider use axis; hinge also
 * uses a perpendicular normal to define zero angle. Hinge limits are radians,
 * slider limits are metres, and limits must bracket zero.
 */
struct PhysicsConstraintDesc {
  PhysicsConstraintType type = PhysicsConstraintType::Fixed;
  PhysicsBodyHandle bodyA{};
  PhysicsBodyHandle bodyB{};
  Vector3 anchor{};
  Vector3 axis{0, 1, 0};
  Vector3 normal{1, 0, 0};
  bool enableLimits = false;
  float limitMin = 0.0f;
  float limitMax = 0.0f;
  PhysicsMotorDesc motor{};
  bool collideConnected = false;
};

/** @brief Character support state after a fixed physics step. */
enum class PhysicsGroundState : std::uint8_t { Airborne, Grounded, Sliding };

/**
 * @brief Describes a Y-up capsule character.
 *
 * height is total capsule height, including both hemispheres; position is
 * the capsule's center.
 */
struct PhysicsCharacterDesc {
  float radius = 0.35f;
  float height = 1.8f;
  Vector3 position{0.0f, 2.0f, 0.0f};
  float maxSlopeAngle = 45.0f;
  float stepHeight = 0.3f;
  float groundSnapDistance = 0.2f;
  std::uint16_t layer = 1;
  std::uint16_t mask = 0xFFFF;
};

}  // namespace Levye
