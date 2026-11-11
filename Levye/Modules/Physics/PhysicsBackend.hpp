#pragma once

#include <Levye/Modules/Physics/PhysicsTypes.hpp>

namespace Levye {

/** @brief Host-side physics backend interface; never exposed through the game ABI. */
class IPhysicsBackend {
 public:
  virtual ~IPhysicsBackend() = default;

  /** @brief Initializes this backend for one physics world. */
  [[nodiscard]] virtual bool Initialize(const PhysicsWorldDesc& desc) = 0;

  /** @brief Releases this backend world and all owned resources. */
  virtual void Shutdown() = 0;

  /** @brief Returns the backend type. */
  [[nodiscard]] virtual PhysicsBackendType GetType() const = 0;

  /** @brief Creates a rigid body and returns its backend handle. */
  [[nodiscard]] virtual PhysicsBodyHandle CreateBody(const PhysicsBodyDesc& desc) = 0;

  /** @brief Destroys a rigid body and invalidates its handle. */
  [[nodiscard]] virtual bool DestroyBody(PhysicsBodyHandle body) = 0;

  /** @brief Reads a rigid body transform in world space. */
  [[nodiscard]] virtual bool GetTransform(PhysicsBodyHandle body,
                                          Transform& outTransform) const = 0;

  /** @brief Sets a rigid body transform in world space. */
  [[nodiscard]] virtual bool SetTransform(PhysicsBodyHandle body, const Transform& transform) = 0;

  /** @brief Sets a rigid body linear velocity. */
  [[nodiscard]] virtual bool SetLinearVelocity(PhysicsBodyHandle body, Vector3 velocity) = 0;

  /** @brief Reads a rigid body linear velocity. */
  [[nodiscard]] virtual bool GetLinearVelocity(PhysicsBodyHandle body,
                                               Vector3& outVelocity) const = 0;

  /** @brief Applies force to a rigid body. */
  [[nodiscard]] virtual bool AddForce(PhysicsBodyHandle body, Vector3 force) = 0;

  /** @brief Applies an instantaneous impulse to a rigid body. */
  [[nodiscard]] virtual bool AddImpulse(PhysicsBodyHandle body, Vector3 impulse) = 0;

  /** @brief Returns the closest ray hit in this world. */
  [[nodiscard]] virtual bool Raycast(Vector3 origin, Vector3 direction, float maxDistance,
                                     PhysicsRaycastHit& outHit) const = 0;

  /** @brief Returns the closest ray hit accepted by the filter. */
  [[nodiscard]] virtual bool RaycastFiltered(Vector3 origin, Vector3 direction, float maxDistance,
                                             const PhysicsRaycastFilter& filter,
                                             PhysicsRaycastHit& outHit) const = 0;

  /**
   * @brief Changes a body's collision category and accepted categories.
   *
   * Must be called outside physics stepping.
   */
  [[nodiscard]] virtual bool SetCollisionFilter(PhysicsBodyHandle body, std::uint16_t layer,
                                                std::uint16_t mask) = 0;

  /** @brief Sweeps a supported convex shape and returns its closest accepted hit. */
  [[nodiscard]] virtual bool ShapeCast(const PhysicsShapeDesc& shape, const Transform& start,
                                       Vector3 direction, float maxDistance,
                                       const PhysicsQueryFilter& filter,
                                       PhysicsShapeCastHit& output) const = 0;

  /** @brief Writes unique overlapping body handles and reports the total hit count. */
  [[nodiscard]] virtual bool OverlapShape(const PhysicsShapeDesc& shape, const Transform& transform,
                                          const PhysicsQueryFilter& filter,
                                          PhysicsBodyHandle* results, std::uint32_t capacity,
                                          std::uint32_t& count) const = 0;

  /** @brief Creates a host-owned virtual capsule character. */
  [[nodiscard]] virtual PhysicsCharacterHandle CreateCharacter(
      const PhysicsCharacterDesc& desc) = 0;
  /** @brief Destroys a character and invalidates its handle. */
  [[nodiscard]] virtual bool DestroyCharacter(PhysicsCharacterHandle character) = 0;
  /** @brief Sets desired horizontal world velocity for fixed stepping. */
  [[nodiscard]] virtual bool SetCharacterVelocity(PhysicsCharacterHandle character,
                                                  Vector3 horizontal) = 0;
  /** @brief Starts an upward jump when the character is grounded. */
  [[nodiscard]] virtual bool JumpCharacter(PhysicsCharacterHandle character, float speed) = 0;
  /** @brief Teleports a character to a world position. */
  [[nodiscard]] virtual bool SetCharacterPosition(PhysicsCharacterHandle character,
                                                  Vector3 position) = 0;
  /** @brief Reads the character capsule center in world space. */
  [[nodiscard]] virtual bool GetCharacterPosition(PhysicsCharacterHandle character,
                                                  Vector3& output) const = 0;
  /** @brief Reads the character’s measured world velocity. */
  [[nodiscard]] virtual bool GetCharacterVelocity(PhysicsCharacterHandle character,
                                                  Vector3& output) const = 0;
  /** @brief Reads the character’s latest support state. */
  [[nodiscard]] virtual bool GetCharacterGroundState(PhysicsCharacterHandle character,
                                                     PhysicsGroundState& output) const = 0;
  /** @brief Reads the character’s latest world-space support normal. */
  [[nodiscard]] virtual bool GetCharacterGroundNormal(PhysicsCharacterHandle character,
                                                      Vector3& output) const = 0;

  /** @brief Creates a constraint between two backend-local body handles. */
  [[nodiscard]] virtual PhysicsConstraintHandle CreateConstraint(
      const PhysicsConstraintDesc& desc) = 0;
  /** @brief Destroys a constraint and invalidates its handle. */
  [[nodiscard]] virtual bool DestroyConstraint(PhysicsConstraintHandle constraint) = 0;
  /** @brief Changes a hinge or slider motor without rebuilding the constraint. */
  [[nodiscard]] virtual bool SetConstraintMotor(PhysicsConstraintHandle constraint,
                                                const PhysicsMotorDesc& motor) = 0;

  /** @brief Retrieves the next queued body-pair contact event. */
  [[nodiscard]] virtual bool PollContactEvent(PhysicsContactEvent& output) = 0;

  /** @brief Number of active rigid-body contact pairs after the last step. */
  [[nodiscard]] virtual std::uint32_t ActiveContactPairCount() const = 0;

  /** @brief Advances this backend by one fixed physics timestep. */
  virtual void Step(float fixedDeltaTime) = 0;
};

}  // namespace Levye
