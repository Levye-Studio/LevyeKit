
#pragma once

#include <Levye/Core/ModuleAPI.hpp>
#include <Levye/Core/Services.hpp>
#include <Levye/Modules/Physics/PhysicsAPI.hpp>

namespace Levye {

/**
 * @brief Access to LevyeKit's optional Physics module.
 *
 * Physics worlds, rigid bodies, and simulation state remain
 * owned by the host and survive game-module hot reloads.
 *
 * All operations are forwarded through the versioned
 * PhysicsAPI function table.
 */
class Physics {
 public:
  /** @brief Returns whether a compatible host Physics API is available. */
  [[nodiscard]] static bool IsAvailable() noexcept {
    const HostServices* host = Services::Get();
    const PhysicsAPI* api = GetAPI(host);

    return api && api->IsAvailable && api->IsAvailable(host->context);
  }

  /** @brief Creates a host-owned physics world. */
  [[nodiscard]] static PhysicsWorldHandle CreateWorld(const PhysicsWorldDesc& desc = {}) {
    const HostServices* host = Services::Get();
    const PhysicsAPI* api = GetAPI(host);

    return api && api->CreateWorld ? api->CreateWorld(host->context, &desc) : PhysicsWorldHandle{};
  }

  /** @brief Destroys a world and all bodies belonging to it. */
  [[nodiscard]] static bool DestroyWorld(PhysicsWorldHandle world) {
    const HostServices* host = Services::Get();
    const PhysicsAPI* api = GetAPI(host);

    return api && api->DestroyWorld && api->DestroyWorld(host->context, world);
  }

  /** @brief Creates a rigid body in a physics world. */
  [[nodiscard]] static PhysicsBodyHandle CreateBody(PhysicsWorldHandle world,
                                                    const PhysicsBodyDesc& desc) {
    const HostServices* host = Services::Get();
    const PhysicsAPI* api = GetAPI(host);

    return api && api->CreateBody ? api->CreateBody(host->context, world, &desc)
                                  : PhysicsBodyHandle{};
  }

  /** @brief Destroys a rigid body. */
  [[nodiscard]] static bool DestroyBody(PhysicsWorldHandle world, PhysicsBodyHandle body) {
    const HostServices* host = Services::Get();
    const PhysicsAPI* api = GetAPI(host);

    return api && api->DestroyBody && api->DestroyBody(host->context, world, body);
  }

  /** @brief Retrieves a body's world transform. */
  [[nodiscard]] static bool GetTransform(PhysicsWorldHandle world, PhysicsBodyHandle body,
                                         Transform& output) {
    const HostServices* host = Services::Get();
    const PhysicsAPI* api = GetAPI(host);

    return api && api->GetTransform && api->GetTransform(host->context, world, body, &output);
  }

  /** @brief Updates a body's world transform. */
  [[nodiscard]] static bool SetTransform(PhysicsWorldHandle world, PhysicsBodyHandle body,
                                         const Transform& transform) {
    const HostServices* host = Services::Get();
    const PhysicsAPI* api = GetAPI(host);

    return api && api->SetTransform && api->SetTransform(host->context, world, body, &transform);
  }

  /** @brief Sets a body's linear velocity. */
  [[nodiscard]] static bool SetLinearVelocity(PhysicsWorldHandle world, PhysicsBodyHandle body,
                                              Vector3 velocity) {
    const HostServices* host = Services::Get();
    const PhysicsAPI* api = GetAPI(host);

    return api && api->SetLinearVelocity &&
           api->SetLinearVelocity(host->context, world, body, velocity);
  }

  /** @brief Retrieves a body's linear velocity. */
  [[nodiscard]] static bool GetLinearVelocity(PhysicsWorldHandle world, PhysicsBodyHandle body,
                                              Vector3& output) {
    const HostServices* host = Services::Get();
    const PhysicsAPI* api = GetAPI(host);

    return api && api->GetLinearVelocity &&
           api->GetLinearVelocity(host->context, world, body, &output);
  }

  /** @brief Applies a force to a rigid body. */
  [[nodiscard]] static bool AddForce(PhysicsWorldHandle world, PhysicsBodyHandle body,
                                     Vector3 force) {
    const HostServices* host = Services::Get();
    const PhysicsAPI* api = GetAPI(host);

    return api && api->AddForce && api->AddForce(host->context, world, body, force);
  }

  /** @brief Applies an instantaneous impulse to a rigid body. */
  [[nodiscard]] static bool AddImpulse(PhysicsWorldHandle world, PhysicsBodyHandle body,
                                       Vector3 impulse) {
    const HostServices* host = Services::Get();
    const PhysicsAPI* api = GetAPI(host);

    return api && api->AddImpulse && api->AddImpulse(host->context, world, body, impulse);
  }

  /** @brief Casts a ray against bodies in the specified world. */
  [[nodiscard]] static bool Raycast(PhysicsWorldHandle world, Vector3 origin, Vector3 direction,
                                    float maxDistance, PhysicsRaycastHit& output) {
    const HostServices* host = Services::Get();
    const PhysicsAPI* api = GetAPI(host);

    return api && api->Raycast &&
           api->Raycast(host->context, world, origin, direction, maxDistance, &output);
  }

  /**
   * @brief Polls a world after fixed stepping on the host thread.
   *
   * Each active body pair emits at most one event per step. Poll regularly:
   * the per-world FIFO holds 8192 events and coalesces nonterminal events on
   * overflow. Terminal End events remain available for reconciliation.
   */
  [[nodiscard]] static bool PollContactEvent(PhysicsWorldHandle world,
                                             PhysicsContactEvent& output) {
    const HostServices* host = Services::Get();
    const PhysicsAPI* api = GetAPI(host);

    return api && api->PollContactEvent && api->PollContactEvent(host->context, world, output);
  }

  /**
   * @brief Casts a ray against selected collision categories.
   *
   * The mask selects target body layers. Sensor bodies can be
   * included or excluded independently.
   *
   * @return true if an accepted body was hit.
   */
  [[nodiscard]] static bool RaycastFiltered(PhysicsWorldHandle world, Vector3 origin,
                                            Vector3 direction, float maxDistance,
                                            const PhysicsRaycastFilter& filter,
                                            PhysicsRaycastHit& output) {
    const HostServices* host = Services::Get();
    const PhysicsAPI* api = GetAPI(host);

    return api && api->RaycastFiltered &&
           api->RaycastFiltered(host->context, world, origin, direction, maxDistance, &filter,
                                &output);
  }

  /**
   * @brief Changes a body's collision category and accepted categories in place.
   *
   * Both bodies must accept one another to collide. A zero mask disables all
   * contacts; a zero layer is invalid. Invalidated pairs emit one End event.
   * Call on the host update thread, outside a physics step.
   */
  [[nodiscard]] static bool SetCollisionFilter(PhysicsWorldHandle world, PhysicsBodyHandle body,
                                               std::uint16_t layer, std::uint16_t mask) {
    const HostServices* host = Services::Get();
    const PhysicsAPI* api = GetAPI(host);
    return api && api->SetCollisionFilter &&
           api->SetCollisionFilter(host->context, world, body, layer, mask);
  }

  /** @brief Sweeps a supported convex shape and returns the closest accepted hit. */
  [[nodiscard]] static bool ShapeCast(PhysicsWorldHandle world, const PhysicsShapeDesc& shape,
                                      const Transform& start, Vector3 direction, float maxDistance,
                                      const PhysicsQueryFilter& filter,
                                      PhysicsShapeCastHit& output) {
    const HostServices* host = Services::Get();
    const PhysicsAPI* api = GetAPI(host);
    return api && api->ShapeCast &&
           api->ShapeCast(host->context, world, &shape, &start, direction, maxDistance, &filter,
                          &output);
  }

  /**
   * @brief Finds unique overlapping bodies in ascending handle order.
   *
   * On success count is the total number of hits. The first min(count, capacity)
   * handles are written to results; count > capacity signals truncation.
   * A null results pointer is valid only with zero capacity.
   */
  [[nodiscard]] static bool OverlapShape(PhysicsWorldHandle world, const PhysicsShapeDesc& shape,
                                         const Transform& transform,
                                         const PhysicsQueryFilter& filter,
                                         PhysicsBodyHandle* results, std::uint32_t capacity,
                                         std::uint32_t& count) {
    const HostServices* host = Services::Get();
    const PhysicsAPI* api = GetAPI(host);
    return api && api->OverlapShape &&
           api->OverlapShape(host->context, world, &shape, &transform, &filter, results, capacity,
                             &count);
  }

  /** @brief Creates a host-owned Y-up capsule character in a physics world. */
  [[nodiscard]] static PhysicsCharacterHandle CreateCharacter(
      PhysicsWorldHandle world, const PhysicsCharacterDesc& desc = {}) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->CreateCharacter ? api->CreateCharacter(host->context, world, &desc)
                                       : PhysicsCharacterHandle{};
  }
  /** @brief Destroys a character and invalidates its handle. */
  [[nodiscard]] static bool DestroyCharacter(PhysicsWorldHandle world,
                                             PhysicsCharacterHandle character) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->DestroyCharacter && api->DestroyCharacter(host->context, world, character);
  }
  /** @brief Sets desired horizontal world velocity; y must be zero. */
  [[nodiscard]] static bool SetCharacterVelocity(PhysicsWorldHandle world,
                                                 PhysicsCharacterHandle character,
                                                 Vector3 horizontal) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->SetCharacterVelocity &&
           api->SetCharacterVelocity(host->context, world, character, horizontal);
  }
  /** @brief Starts an upward jump from walkable ground at the supplied speed. */
  [[nodiscard]] static bool JumpCharacter(PhysicsWorldHandle world,
                                          PhysicsCharacterHandle character, float speed) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->JumpCharacter && api->JumpCharacter(host->context, world, character, speed);
  }
  /** @brief Teleports a character; use for reset or explicit relocation. */
  [[nodiscard]] static bool SetCharacterPosition(PhysicsWorldHandle world,
                                                 PhysicsCharacterHandle character,
                                                 Vector3 position) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->SetCharacterPosition &&
           api->SetCharacterPosition(host->context, world, character, position);
  }
  /** @brief Returns the capsule center in world space. */
  [[nodiscard]] static bool GetCharacterPosition(PhysicsWorldHandle world,
                                                 PhysicsCharacterHandle character,
                                                 Vector3& output) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->GetCharacterPosition &&
           api->GetCharacterPosition(host->context, world, character, &output);
  }
  /** @brief Returns the current simulated velocity, including vertical motion. */
  [[nodiscard]] static bool GetCharacterVelocity(PhysicsWorldHandle world,
                                                 PhysicsCharacterHandle character,
                                                 Vector3& output) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->GetCharacterVelocity &&
           api->GetCharacterVelocity(host->context, world, character, &output);
  }
  /** @brief Reports walkable ground, a steep support, or airborne state. */
  [[nodiscard]] static bool GetCharacterGroundState(PhysicsWorldHandle world,
                                                    PhysicsCharacterHandle character,
                                                    PhysicsGroundState& output) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->GetCharacterGroundState &&
           api->GetCharacterGroundState(host->context, world, character, &output);
  }
  /** @brief Returns the latest world-space support normal. */
  [[nodiscard]] static bool GetCharacterGroundNormal(PhysicsWorldHandle world,
                                                     PhysicsCharacterHandle character,
                                                     Vector3& output) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->GetCharacterGroundNormal &&
           api->GetCharacterGroundNormal(host->context, world, character, &output);
  }

  /** @brief Pauses or resumes automatic fixed stepping of one world. */
  [[nodiscard]] static bool SetWorldPaused(PhysicsWorldHandle world, bool paused) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->SetWorldPaused && api->SetWorldPaused(host->context, world, paused);
  }

  /** @brief Queues one step for a paused world on the next host fixed tick. */
  [[nodiscard]] static bool StepWorldOnce(PhysicsWorldHandle world) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->StepWorldOnce && api->StepWorldOnce(host->context, world);
  }

  /** @brief Reads body, character, contact, and timing counters for a world. */
  [[nodiscard]] static bool GetWorldDiagnostics(PhysicsWorldHandle world,
                                                PhysicsWorldDiagnostics& output) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->GetWorldDiagnostics &&
           api->GetWorldDiagnostics(host->context, world, &output);
  }

  /** @brief Creates a host-owned fixed, point, hinge, or slider constraint. */
  [[nodiscard]] static PhysicsConstraintHandle CreateConstraint(PhysicsWorldHandle world,
                                                                const PhysicsConstraintDesc& desc) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->CreateConstraint ? api->CreateConstraint(host->context, world, &desc)
                                        : PhysicsConstraintHandle{};
  }

  /** @brief Destroys a constraint; destroying either body also removes it. */
  [[nodiscard]] static bool DestroyConstraint(PhysicsWorldHandle world,
                                              PhysicsConstraintHandle constraint) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->DestroyConstraint &&
           api->DestroyConstraint(host->context, world, constraint);
  }

  /** @brief Changes a hinge or slider motor target and limits. */
  [[nodiscard]] static bool SetConstraintMotor(PhysicsWorldHandle world,
                                               PhysicsConstraintHandle constraint,
                                               const PhysicsMotorDesc& motor) {
    const auto* host = Services::Get();
    const auto* api = GetAPI(host);
    return api && api->SetConstraintMotor &&
           api->SetConstraintMotor(host->context, world, constraint, &motor);
  }

 private:
  Physics() = delete;

  /**
   * @brief Resolves and validates the host Physics API.
   *
   * The API is resolved for each operation instead of cached,
   * keeping module bindings safe across hot reloads.
   */
  [[nodiscard]] static const PhysicsAPI* GetAPI(const HostServices* host) {
    if (!host || !host->GetModuleAPI) {
      return nullptr;
    }

    const auto* api = static_cast<const PhysicsAPI*>(
        host->GetModuleAPI(host->context, ModuleID::Physics, PHYSICS_API_VERSION));

    return api && api->version == PHYSICS_API_VERSION ? api : nullptr;
  }
};

}  // namespace Levye
