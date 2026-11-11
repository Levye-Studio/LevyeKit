
#pragma once

#include <Levye/Modules/Physics/ContactPairRegistry.hpp>
#include <Levye/Modules/Physics/PhysicsBackend.hpp>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Levye {

/**
 * @brief Host-owned manager for physics worlds.
 *
 * Owns all physics backends and their associated worlds.
 * Its lifetime must be independent of the reloadable game module.
 *
 * Worlds are identified by generation-checked handles.
 * Public calls, stepping and polling are serialized by the host main thread;
 * Jolt worker callbacks only record observations in the backend listener.
 */
class PhysicsService {
 public:
  PhysicsService() = default;
  ~PhysicsService();

  PhysicsService(const PhysicsService&) = delete;
  PhysicsService& operator=(const PhysicsService&) = delete;

  /**
   * @brief Creates a world using the requested physics backend.
   *
   * @return Valid world handle on success, invalid handle otherwise.
   */
  [[nodiscard]] PhysicsWorldHandle CreateWorld(const PhysicsWorldDesc& desc);

  /**
   * @brief Destroys a world and all bodies belonging to it.
   */
  [[nodiscard]] bool DestroyWorld(PhysicsWorldHandle world);

  /** @brief Pauses or resumes automatic fixed stepping for one world. */
  [[nodiscard]] bool SetWorldPaused(PhysicsWorldHandle world, bool paused);
  /** @brief Queues one fixed step for a paused world on the next host fixed tick. */
  [[nodiscard]] bool StepWorldOnce(PhysicsWorldHandle world);
  /** @brief Reads host-owned world counters and last-step timing. */
  [[nodiscard]] bool GetWorldDiagnostics(PhysicsWorldHandle world,
                                         PhysicsWorldDiagnostics& output) const;

  /**
   * @brief Creates a rigid body in an existing world.
   */
  [[nodiscard]] PhysicsBodyHandle CreateBody(PhysicsWorldHandle world, const PhysicsBodyDesc& desc);

  /** @brief Destroys a rigid body and invalidates its handle. */
  [[nodiscard]] bool DestroyBody(PhysicsWorldHandle world, PhysicsBodyHandle body);

  /** @brief Reads a rigid body transform in world space. */
  [[nodiscard]] bool GetTransform(PhysicsWorldHandle world, PhysicsBodyHandle body,
                                  Transform& output) const;

  /** @brief Sets a rigid body transform in world space. */
  [[nodiscard]] bool SetTransform(PhysicsWorldHandle world, PhysicsBodyHandle body,
                                  const Transform& transform);

  /** @brief Sets a rigid body linear velocity. */
  [[nodiscard]] bool SetLinearVelocity(PhysicsWorldHandle world, PhysicsBodyHandle body,
                                       Vector3 velocity);

  /** @brief Reads a rigid body linear velocity. */
  [[nodiscard]] bool GetLinearVelocity(PhysicsWorldHandle world, PhysicsBodyHandle body,
                                       Vector3& output) const;

  /** @brief Applies force to a rigid body. */
  [[nodiscard]] bool AddForce(PhysicsWorldHandle world, PhysicsBodyHandle body, Vector3 force);

  /** @brief Applies an instantaneous impulse to a rigid body. */
  [[nodiscard]] bool AddImpulse(PhysicsWorldHandle world, PhysicsBodyHandle body, Vector3 impulse);

  /** @brief Returns the closest ray hit in this world. */
  [[nodiscard]] bool Raycast(PhysicsWorldHandle world, Vector3 origin, Vector3 direction,
                             float maxDistance, PhysicsRaycastHit& output) const;

  /** @brief Retrieves the next queued body-pair contact event. */
  [[nodiscard]] bool PollContactEvent(PhysicsWorldHandle world, PhysicsContactEvent& output);

  /** @brief Returns the closest ray hit accepted by the filter. */
  [[nodiscard]] bool RaycastFiltered(PhysicsWorldHandle world, Vector3 origin, Vector3 direction,
                                     float maxDistance, const PhysicsRaycastFilter& filter,
                                     PhysicsRaycastHit& output) const;

  /** @brief Changes a body's layer and mask without recreating it. */
  [[nodiscard]] bool SetCollisionFilter(PhysicsWorldHandle world, PhysicsBodyHandle body,
                                        std::uint16_t layer, std::uint16_t mask);

  /** @brief Sweeps a supported convex shape and returns its closest accepted hit. */
  [[nodiscard]] bool ShapeCast(PhysicsWorldHandle world, const PhysicsShapeDesc& shape,
                               const Transform& start, Vector3 direction, float maxDistance,
                               const PhysicsQueryFilter& filter, PhysicsShapeCastHit& output) const;

  /** @brief Writes unique overlapping body handles and reports the total hit count. */
  [[nodiscard]] bool OverlapShape(PhysicsWorldHandle world, const PhysicsShapeDesc& shape,
                                  const Transform& transform, const PhysicsQueryFilter& filter,
                                  PhysicsBodyHandle* results, std::uint32_t capacity,
                                  std::uint32_t& count) const;

  /** @brief Creates a host-owned virtual capsule character. */
  [[nodiscard]] PhysicsCharacterHandle CreateCharacter(PhysicsWorldHandle world,
                                                       const PhysicsCharacterDesc& desc);
  /** @brief Destroys a character and invalidates its handle. */
  [[nodiscard]] bool DestroyCharacter(PhysicsWorldHandle world, PhysicsCharacterHandle character);
  /** @brief Sets desired horizontal world velocity for fixed stepping. */
  [[nodiscard]] bool SetCharacterVelocity(PhysicsWorldHandle world,
                                          PhysicsCharacterHandle character, Vector3 horizontal);
  /** @brief Starts an upward jump when the character is grounded. */
  [[nodiscard]] bool JumpCharacter(PhysicsWorldHandle world, PhysicsCharacterHandle character,
                                   float speed);
  /** @brief Teleports a character to a world position. */
  [[nodiscard]] bool SetCharacterPosition(PhysicsWorldHandle world,
                                          PhysicsCharacterHandle character, Vector3 position);
  /** @brief Reads the character capsule center in world space. */
  [[nodiscard]] bool GetCharacterPosition(PhysicsWorldHandle world,
                                          PhysicsCharacterHandle character, Vector3& output) const;
  /** @brief Reads the character’s measured world velocity. */
  [[nodiscard]] bool GetCharacterVelocity(PhysicsWorldHandle world,
                                          PhysicsCharacterHandle character, Vector3& output) const;
  /** @brief Reads the character’s latest support state. */
  [[nodiscard]] bool GetCharacterGroundState(PhysicsWorldHandle world,
                                             PhysicsCharacterHandle character,
                                             PhysicsGroundState& output) const;
  /** @brief Reads the character’s latest world-space support normal. */
  [[nodiscard]] bool GetCharacterGroundNormal(PhysicsWorldHandle world,
                                              PhysicsCharacterHandle character,
                                              Vector3& output) const;

  /** @brief Creates a host-owned two-body constraint. */
  [[nodiscard]] PhysicsConstraintHandle CreateConstraint(PhysicsWorldHandle world,
                                                         const PhysicsConstraintDesc& desc);
  /** @brief Destroys a constraint and invalidates its public handle. */
  [[nodiscard]] bool DestroyConstraint(PhysicsWorldHandle world,
                                       PhysicsConstraintHandle constraint);
  /** @brief Changes a hinge or slider motor. */
  [[nodiscard]] bool SetConstraintMotor(PhysicsWorldHandle world,
                                        PhysicsConstraintHandle constraint,
                                        const PhysicsMotorDesc& motor);

  /**
   * @brief Advances all active worlds by one fixed timestep.
   *
   * Called by the host, never by the game module.
   */
  void Step(float fixedDeltaTime);

  /**
   * @brief Releases every world and backend.
   *
   * Call only during host shutdown or explicit service reset.
   */
  void Shutdown();

 private:
  struct WorldSlot {
    struct ConstraintRecord {
      PhysicsConstraintHandle backend{};
      PhysicsBodyHandle bodyA{};
      PhysicsBodyHandle bodyB{};
    };
    std::unique_ptr<IPhysicsBackend> backend;
    std::uint32_t generation = 1;

    // Maps public body handles to backend-local body handles.
    std::unordered_map<std::uint64_t, PhysicsBodyHandle> bodies;
    std::unordered_map<std::uint64_t, PhysicsBodyHandle> publicByBackend;
    std::unordered_map<std::uint64_t, PhysicsCharacterHandle> characters;
    std::unordered_map<std::uint64_t, ConstraintRecord> constraints;
    ContactEventQueue contactEvents;
    bool paused = false;
    std::uint32_t pendingSingleSteps = 0;
    std::uint64_t completedSteps = 0;
    std::uint64_t rejectedCreations = 0;
    double lastStepMilliseconds = 0.0;
  };

  [[nodiscard]] WorldSlot* ResolveWorld(PhysicsWorldHandle world);
  [[nodiscard]] const WorldSlot* ResolveWorld(PhysicsWorldHandle world) const;

  [[nodiscard]] static std::uint64_t BodyKey(PhysicsBodyHandle handle);

  [[nodiscard]] PhysicsBodyHandle RegisterBody(WorldSlot& slot, PhysicsBodyHandle backendHandle);

  [[nodiscard]] PhysicsBodyHandle FindPublicBody(const WorldSlot& slot,
                                                 PhysicsBodyHandle backendHandle) const;
  void DrainBackendContacts(WorldSlot& slot);

  [[nodiscard]] const PhysicsBodyHandle* ResolveBody(const WorldSlot& slot,
                                                     PhysicsBodyHandle publicHandle) const;
  [[nodiscard]] const PhysicsCharacterHandle* ResolveCharacter(
      const WorldSlot& slot, PhysicsCharacterHandle publicHandle) const;

 private:
  std::vector<WorldSlot> m_Worlds;
  std::vector<std::uint32_t> m_FreeWorlds;

  std::uint64_t m_NextBodyId = 1;
  std::uint64_t m_NextCharacterId = 1;
  std::uint64_t m_NextConstraintId = 1;
};

}  // namespace Levye
