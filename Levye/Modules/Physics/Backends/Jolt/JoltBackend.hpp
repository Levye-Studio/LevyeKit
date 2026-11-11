#pragma once

#include <Levye/Modules/Physics/PhysicsBackend.hpp>
#include <memory>

namespace Levye {

/** @brief Host-owned Jolt implementation of the physics backend contract. */
class JoltBackend final : public IPhysicsBackend {
 public:
  JoltBackend();
  ~JoltBackend() override;

  JoltBackend(const JoltBackend&) = delete;
  JoltBackend& operator=(const JoltBackend&) = delete;

  /** @brief Initializes one Jolt physics world. */
  [[nodiscard]] bool Initialize(const PhysicsWorldDesc& desc) override;
  /** @brief Releases the Jolt world and its resources. */
  void Shutdown() override;

  /** @brief Returns the Jolt backend identifier. */
  [[nodiscard]] PhysicsBackendType GetType() const override;

  /** @brief Creates a Jolt rigid body. */
  [[nodiscard]] PhysicsBodyHandle CreateBody(const PhysicsBodyDesc& desc) override;

  /** @brief Destroys a Jolt rigid body. */
  [[nodiscard]] bool DestroyBody(PhysicsBodyHandle body) override;

  /** @brief Reads a body transform. */
  [[nodiscard]] bool GetTransform(PhysicsBodyHandle body, Transform& outTransform) const override;

  /** @brief Sets a body transform. */
  [[nodiscard]] bool SetTransform(PhysicsBodyHandle body, const Transform& transform) override;

  /** @brief Sets body linear velocity. */
  [[nodiscard]] bool SetLinearVelocity(PhysicsBodyHandle body, Vector3 velocity) override;

  /** @brief Reads body linear velocity. */
  [[nodiscard]] bool GetLinearVelocity(PhysicsBodyHandle body, Vector3& outVelocity) const override;

  /** @brief Applies body force. */
  [[nodiscard]] bool AddForce(PhysicsBodyHandle body, Vector3 force) override;

  /** @brief Applies a body impulse. */
  [[nodiscard]] bool AddImpulse(PhysicsBodyHandle body, Vector3 impulse) override;

  /** @brief Finds the nearest ray hit. */
  [[nodiscard]] bool Raycast(Vector3 origin, Vector3 direction, float maxDistance,
                             PhysicsRaycastHit& outHit) const override;

  /** @brief Finds the nearest ray hit accepted by a filter. */
  [[nodiscard]] bool RaycastFiltered(Vector3 origin, Vector3 direction, float maxDistance,
                                     const PhysicsRaycastFilter& filter,
                                     PhysicsRaycastHit& outHit) const override;

  /** @brief Changes a body collision layer and mask in place. */
  [[nodiscard]] bool SetCollisionFilter(PhysicsBodyHandle body, std::uint16_t layer,
                                        std::uint16_t mask) override;

  /** @brief Sweeps a supported convex shape against the world. */
  [[nodiscard]] bool ShapeCast(const PhysicsShapeDesc& shape, const Transform& start,
                               Vector3 direction, float maxDistance,
                               const PhysicsQueryFilter& filter,
                               PhysicsShapeCastHit& output) const override;
  /** @brief Collects unique bodies overlapping a supported shape. */
  [[nodiscard]] bool OverlapShape(const PhysicsShapeDesc& shape, const Transform& transform,
                                  const PhysicsQueryFilter& filter, PhysicsBodyHandle* results,
                                  std::uint32_t capacity, std::uint32_t& count) const override;

  /** @brief Creates a host-owned Jolt virtual character. */
  [[nodiscard]] PhysicsCharacterHandle CreateCharacter(const PhysicsCharacterDesc& desc) override;
  /** @brief Destroys a virtual character. */
  [[nodiscard]] bool DestroyCharacter(PhysicsCharacterHandle character) override;
  /** @brief Sets desired horizontal character velocity. */
  [[nodiscard]] bool SetCharacterVelocity(PhysicsCharacterHandle character,
                                          Vector3 horizontal) override;
  /** @brief Starts a grounded character jump. */
  [[nodiscard]] bool JumpCharacter(PhysicsCharacterHandle character, float speed) override;
  /** @brief Teleports a virtual character. */
  [[nodiscard]] bool SetCharacterPosition(PhysicsCharacterHandle character,
                                          Vector3 position) override;
  /** @brief Reads the character capsule center. */
  [[nodiscard]] bool GetCharacterPosition(PhysicsCharacterHandle character,
                                          Vector3& output) const override;
  /** @brief Reads measured character velocity. */
  [[nodiscard]] bool GetCharacterVelocity(PhysicsCharacterHandle character,
                                          Vector3& output) const override;
  /** @brief Reads the character support state. */
  [[nodiscard]] bool GetCharacterGroundState(PhysicsCharacterHandle character,
                                             PhysicsGroundState& output) const override;
  /** @brief Reads the character support normal. */
  [[nodiscard]] bool GetCharacterGroundNormal(PhysicsCharacterHandle character,
                                              Vector3& output) const override;
  /** @brief Creates a Jolt two-body constraint. */
  [[nodiscard]] PhysicsConstraintHandle CreateConstraint(
      const PhysicsConstraintDesc& desc) override;
  /** @brief Removes a Jolt constraint. */
  [[nodiscard]] bool DestroyConstraint(PhysicsConstraintHandle constraint) override;
  /** @brief Changes a hinge or slider motor. */
  [[nodiscard]] bool SetConstraintMotor(PhysicsConstraintHandle constraint,
                                        const PhysicsMotorDesc& motor) override;

  /**
   * @brief Retrieves the next pending contact event.
   *
   * Returns backend-local body handles. PhysicsService translates
   * them into public handles before returning them to game code.
   */
  [[nodiscard]] bool PollContactEvent(PhysicsContactEvent& output) override;
  /** @brief Returns active body-pair contacts. */
  [[nodiscard]] std::uint32_t ActiveContactPairCount() const override;

  /** @brief Advances the Jolt world and its characters by one fixed step. */
  void Step(float fixedDeltaTime) override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace Levye
