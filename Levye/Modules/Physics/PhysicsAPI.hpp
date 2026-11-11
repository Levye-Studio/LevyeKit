
#pragma once

#include <Levye/Core/ModuleAPI.hpp>
#include <Levye/Modules/Physics/PhysicsTypes.hpp>
#include <cstdint>
#include <type_traits>

namespace Levye {

inline constexpr ModuleAPIVersion PHYSICS_API_VERSION = 7;

/**
 * @brief Versioned function table for the optional Physics module.
 *
 * All function pointers refer to host-owned code.
 * Game modules must resolve this table through HostServices.
 *
 * No Jolt objects or backend-specific pointers cross this boundary.
 */
struct PhysicsAPI {
  ModuleAPIVersion version = PHYSICS_API_VERSION;

  bool (*IsAvailable)(void* context) = nullptr;

  PhysicsWorldHandle (*CreateWorld)(void* context, const PhysicsWorldDesc* desc) = nullptr;

  bool (*DestroyWorld)(void* context, PhysicsWorldHandle world) = nullptr;

  PhysicsBodyHandle (*CreateBody)(void* context, PhysicsWorldHandle world,
                                  const PhysicsBodyDesc* desc) = nullptr;

  bool (*DestroyBody)(void* context, PhysicsWorldHandle world, PhysicsBodyHandle body) = nullptr;

  bool (*GetTransform)(void* context, PhysicsWorldHandle world, PhysicsBodyHandle body,
                       Transform* output) = nullptr;

  bool (*SetTransform)(void* context, PhysicsWorldHandle world, PhysicsBodyHandle body,
                       const Transform* transform) = nullptr;

  bool (*SetLinearVelocity)(void* context, PhysicsWorldHandle world, PhysicsBodyHandle body,
                            Vector3 velocity) = nullptr;

  bool (*GetLinearVelocity)(void* context, PhysicsWorldHandle world, PhysicsBodyHandle body,
                            Vector3* output) = nullptr;

  bool (*AddForce)(void* context, PhysicsWorldHandle world, PhysicsBodyHandle body,
                   Vector3 force) = nullptr;

  bool (*AddImpulse)(void* context, PhysicsWorldHandle world, PhysicsBodyHandle body,
                     Vector3 impulse) = nullptr;

  bool (*Raycast)(void* context, PhysicsWorldHandle world, Vector3 origin, Vector3 direction,
                  float maxDistance, PhysicsRaycastHit* output) = nullptr;

  bool (*PollContactEvent)(void* context, PhysicsWorldHandle world,
                           PhysicsContactEvent& output) = nullptr;

  bool (*RaycastFiltered)(void* context, PhysicsWorldHandle world, Vector3 origin,
                          Vector3 direction, float maxDistance, const PhysicsRaycastFilter* filter,
                          PhysicsRaycastHit* output) = nullptr;

  bool (*SetCollisionFilter)(void* context, PhysicsWorldHandle world, PhysicsBodyHandle body,
                             std::uint16_t layer, std::uint16_t mask) = nullptr;

  bool (*ShapeCast)(void* context, PhysicsWorldHandle world, const PhysicsShapeDesc* shape,
                    const Transform* start, Vector3 direction, float maxDistance,
                    const PhysicsQueryFilter* filter, PhysicsShapeCastHit* output) = nullptr;

  bool (*OverlapShape)(void* context, PhysicsWorldHandle world, const PhysicsShapeDesc* shape,
                       const Transform* transform, const PhysicsQueryFilter* filter,
                       PhysicsBodyHandle* results, std::uint32_t capacity,
                       std::uint32_t* count) = nullptr;

  PhysicsCharacterHandle (*CreateCharacter)(void* context, PhysicsWorldHandle world,
                                            const PhysicsCharacterDesc* desc) = nullptr;
  bool (*DestroyCharacter)(void* context, PhysicsWorldHandle world,
                           PhysicsCharacterHandle character) = nullptr;
  bool (*SetCharacterVelocity)(void* context, PhysicsWorldHandle world,
                               PhysicsCharacterHandle character, Vector3 horizontal) = nullptr;
  bool (*JumpCharacter)(void* context, PhysicsWorldHandle world, PhysicsCharacterHandle character,
                        float speed) = nullptr;
  bool (*SetCharacterPosition)(void* context, PhysicsWorldHandle world,
                               PhysicsCharacterHandle character, Vector3 position) = nullptr;
  bool (*GetCharacterPosition)(void* context, PhysicsWorldHandle world,
                               PhysicsCharacterHandle character, Vector3* output) = nullptr;
  bool (*GetCharacterVelocity)(void* context, PhysicsWorldHandle world,
                               PhysicsCharacterHandle character, Vector3* output) = nullptr;
  bool (*GetCharacterGroundState)(void* context, PhysicsWorldHandle world,
                                  PhysicsCharacterHandle character,
                                  PhysicsGroundState* output) = nullptr;
  bool (*GetCharacterGroundNormal)(void* context, PhysicsWorldHandle world,
                                   PhysicsCharacterHandle character, Vector3* output) = nullptr;

  bool (*SetWorldPaused)(void* context, PhysicsWorldHandle world, bool paused) = nullptr;
  bool (*StepWorldOnce)(void* context, PhysicsWorldHandle world) = nullptr;
  bool (*GetWorldDiagnostics)(void* context, PhysicsWorldHandle world,
                              PhysicsWorldDiagnostics* output) = nullptr;

  PhysicsConstraintHandle (*CreateConstraint)(void* context, PhysicsWorldHandle world,
                                              const PhysicsConstraintDesc* desc) = nullptr;
  bool (*DestroyConstraint)(void* context, PhysicsWorldHandle world,
                            PhysicsConstraintHandle constraint) = nullptr;
  bool (*SetConstraintMotor)(void* context, PhysicsWorldHandle world,
                             PhysicsConstraintHandle constraint,
                             const PhysicsMotorDesc* motor) = nullptr;
};

static_assert(std::is_standard_layout_v<PhysicsAPI>);
static_assert(std::is_trivially_copyable_v<PhysicsAPI>);
static_assert(std::is_trivially_copyable_v<PhysicsCharacterHandle>);

}  // namespace Levye
