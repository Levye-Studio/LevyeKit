
#include "PhysicsBridge.hpp"

#include <Levye/Core/HostContext.hpp>
#include <Levye/Modules/Physics/PhysicsService.hpp>

namespace Levye {
namespace {

PhysicsService* APIGetPhysics(void* context) {
  if (!context) return nullptr;

  auto* host = static_cast<HostContext*>(context);
  return host->physics;
}

bool APIIsAvailable(void* context) { return APIGetPhysics(context) != nullptr; }

PhysicsWorldHandle APICreateWorld(void* context, const PhysicsWorldDesc* desc) {
  auto* physics = APIGetPhysics(context);
  return physics && desc ? physics->CreateWorld(*desc) : PhysicsWorldHandle{};
}

bool APIDestroyWorld(void* context, PhysicsWorldHandle world) {
  auto* physics = APIGetPhysics(context);
  return physics && physics->DestroyWorld(world);
}

PhysicsBodyHandle APICreateBody(void* context, PhysicsWorldHandle world,
                                const PhysicsBodyDesc* desc) {
  auto* physics = APIGetPhysics(context);
  return physics && desc ? physics->CreateBody(world, *desc) : PhysicsBodyHandle{};
}

bool APIDestroyBody(void* context, PhysicsWorldHandle world, PhysicsBodyHandle body) {
  auto* physics = APIGetPhysics(context);
  return physics && physics->DestroyBody(world, body);
}

bool APIGetTransform(void* context, PhysicsWorldHandle world, PhysicsBodyHandle body,
                     Transform* output) {
  auto* physics = APIGetPhysics(context);
  return physics && output && physics->GetTransform(world, body, *output);
}

bool APISetTransform(void* context, PhysicsWorldHandle world, PhysicsBodyHandle body,
                     const Transform* transform) {
  auto* physics = APIGetPhysics(context);
  return physics && transform && physics->SetTransform(world, body, *transform);
}

bool APISetLinearVelocity(void* context, PhysicsWorldHandle world, PhysicsBodyHandle body,
                          Vector3 velocity) {
  auto* physics = APIGetPhysics(context);
  return physics && physics->SetLinearVelocity(world, body, velocity);
}

bool APIGetLinearVelocity(void* context, PhysicsWorldHandle world, PhysicsBodyHandle body,
                          Vector3* output) {
  auto* physics = APIGetPhysics(context);
  return physics && output && physics->GetLinearVelocity(world, body, *output);
}

bool APIAddForce(void* context, PhysicsWorldHandle world, PhysicsBodyHandle body, Vector3 force) {
  auto* physics = APIGetPhysics(context);
  return physics && physics->AddForce(world, body, force);
}

bool APIAddImpulse(void* context, PhysicsWorldHandle world, PhysicsBodyHandle body,
                   Vector3 impulse) {
  auto* physics = APIGetPhysics(context);
  return physics && physics->AddImpulse(world, body, impulse);
}

bool APIRaycast(void* context, PhysicsWorldHandle world, Vector3 origin, Vector3 direction,
                float maxDistance, PhysicsRaycastHit* output) {
  auto* physics = APIGetPhysics(context);
  return physics && output && physics->Raycast(world, origin, direction, maxDistance, *output);
}

bool APIPollContactEvent(void* context, PhysicsWorldHandle world, PhysicsContactEvent& output) {
  auto* physics = APIGetPhysics(context);
  return physics && physics->PollContactEvent(world, output);
}

bool APIRaycastFiltered(void* context, PhysicsWorldHandle world, Vector3 origin, Vector3 direction,
                        float maxDistance, const PhysicsRaycastFilter* filter,
                        PhysicsRaycastHit* output) {
  auto* physics = APIGetPhysics(context);

  return physics && filter && output &&
         physics->RaycastFiltered(world, origin, direction, maxDistance, *filter, *output);
}

bool APISetCollisionFilter(void* context, PhysicsWorldHandle world, PhysicsBodyHandle body,
                           std::uint16_t layer, std::uint16_t mask) {
  auto* physics = APIGetPhysics(context);

  return physics && physics->SetCollisionFilter(world, body, layer, mask);
}

bool APIShapeCast(void* context, PhysicsWorldHandle world, const PhysicsShapeDesc* shape,
                  const Transform* start, Vector3 direction, float maxDistance,
                  const PhysicsQueryFilter* filter, PhysicsShapeCastHit* output) {
  auto* physics = APIGetPhysics(context);
  return physics && shape && start && filter && output &&
         physics->ShapeCast(world, *shape, *start, direction, maxDistance, *filter, *output);
}

bool APIOverlapShape(void* context, PhysicsWorldHandle world, const PhysicsShapeDesc* shape,
                     const Transform* transform, const PhysicsQueryFilter* filter,
                     PhysicsBodyHandle* results, std::uint32_t capacity, std::uint32_t* count) {
  auto* physics = APIGetPhysics(context);
  return physics && shape && transform && filter && count && (capacity == 0 || results) &&
         physics->OverlapShape(world, *shape, *transform, *filter, results, capacity, *count);
}

PhysicsCharacterHandle APICreateCharacter(void* context, PhysicsWorldHandle world,
                                          const PhysicsCharacterDesc* desc) {
  auto* physics = APIGetPhysics(context);
  return physics && desc ? physics->CreateCharacter(world, *desc) : PhysicsCharacterHandle{};
}
bool APIDestroyCharacter(void* context, PhysicsWorldHandle world,
                         PhysicsCharacterHandle character) {
  auto* physics = APIGetPhysics(context);
  return physics && physics->DestroyCharacter(world, character);
}
bool APISetCharacterVelocity(void* context, PhysicsWorldHandle world,
                             PhysicsCharacterHandle character, Vector3 horizontal) {
  auto* physics = APIGetPhysics(context);
  return physics && physics->SetCharacterVelocity(world, character, horizontal);
}
bool APIJumpCharacter(void* context, PhysicsWorldHandle world, PhysicsCharacterHandle character,
                      float speed) {
  auto* physics = APIGetPhysics(context);
  return physics && physics->JumpCharacter(world, character, speed);
}
bool APISetCharacterPosition(void* context, PhysicsWorldHandle world,
                             PhysicsCharacterHandle character, Vector3 position) {
  auto* physics = APIGetPhysics(context);
  return physics && physics->SetCharacterPosition(world, character, position);
}
bool APIGetCharacterPosition(void* context, PhysicsWorldHandle world,
                             PhysicsCharacterHandle character, Vector3* output) {
  auto* physics = APIGetPhysics(context);
  return physics && output && physics->GetCharacterPosition(world, character, *output);
}
bool APIGetCharacterVelocity(void* context, PhysicsWorldHandle world,
                             PhysicsCharacterHandle character, Vector3* output) {
  auto* physics = APIGetPhysics(context);
  return physics && output && physics->GetCharacterVelocity(world, character, *output);
}
bool APIGetCharacterGroundState(void* context, PhysicsWorldHandle world,
                                PhysicsCharacterHandle character, PhysicsGroundState* output) {
  auto* physics = APIGetPhysics(context);
  return physics && output && physics->GetCharacterGroundState(world, character, *output);
}
bool APIGetCharacterGroundNormal(void* context, PhysicsWorldHandle world,
                                 PhysicsCharacterHandle character, Vector3* output) {
  auto* physics = APIGetPhysics(context);
  return physics && output && physics->GetCharacterGroundNormal(world, character, *output);
}

bool APISetWorldPaused(void* context, PhysicsWorldHandle world, bool paused) {
  auto* physics = APIGetPhysics(context);
  return physics && physics->SetWorldPaused(world, paused);
}
bool APIStepWorldOnce(void* context, PhysicsWorldHandle world) {
  auto* physics = APIGetPhysics(context);
  return physics && physics->StepWorldOnce(world);
}
bool APIGetWorldDiagnostics(void* context, PhysicsWorldHandle world,
                            PhysicsWorldDiagnostics* output) {
  auto* physics = APIGetPhysics(context);
  return physics && output && physics->GetWorldDiagnostics(world, *output);
}

PhysicsConstraintHandle APICreateConstraint(void* context, PhysicsWorldHandle world,
                                            const PhysicsConstraintDesc* desc) {
  auto* physics = APIGetPhysics(context);
  return physics && desc ? physics->CreateConstraint(world, *desc) : PhysicsConstraintHandle{};
}
bool APIDestroyConstraint(void* context, PhysicsWorldHandle world,
                          PhysicsConstraintHandle constraint) {
  auto* physics = APIGetPhysics(context);
  return physics && physics->DestroyConstraint(world, constraint);
}
bool APISetConstraintMotor(void* context, PhysicsWorldHandle world,
                           PhysicsConstraintHandle constraint, const PhysicsMotorDesc* motor) {
  auto* physics = APIGetPhysics(context);
  return physics && motor && physics->SetConstraintMotor(world, constraint, *motor);
}

}  // namespace

PhysicsAPI MakePhysicsAPI() {
  PhysicsAPI api{};

  api.version = PHYSICS_API_VERSION;
  api.IsAvailable = APIIsAvailable;
  api.CreateWorld = APICreateWorld;
  api.DestroyWorld = APIDestroyWorld;
  api.CreateBody = APICreateBody;
  api.DestroyBody = APIDestroyBody;
  api.GetTransform = APIGetTransform;
  api.SetTransform = APISetTransform;
  api.SetLinearVelocity = APISetLinearVelocity;
  api.GetLinearVelocity = APIGetLinearVelocity;
  api.AddForce = APIAddForce;
  api.AddImpulse = APIAddImpulse;
  api.Raycast = APIRaycast;
  api.PollContactEvent = APIPollContactEvent;
  api.RaycastFiltered = APIRaycastFiltered;
  api.SetCollisionFilter = APISetCollisionFilter;
  api.ShapeCast = APIShapeCast;
  api.OverlapShape = APIOverlapShape;
  api.CreateCharacter = APICreateCharacter;
  api.DestroyCharacter = APIDestroyCharacter;
  api.SetCharacterVelocity = APISetCharacterVelocity;
  api.JumpCharacter = APIJumpCharacter;
  api.SetCharacterPosition = APISetCharacterPosition;
  api.GetCharacterPosition = APIGetCharacterPosition;
  api.GetCharacterVelocity = APIGetCharacterVelocity;
  api.GetCharacterGroundState = APIGetCharacterGroundState;
  api.GetCharacterGroundNormal = APIGetCharacterGroundNormal;
  api.SetWorldPaused = APISetWorldPaused;
  api.StepWorldOnce = APIStepWorldOnce;
  api.GetWorldDiagnostics = APIGetWorldDiagnostics;
  api.CreateConstraint = APICreateConstraint;
  api.DestroyConstraint = APIDestroyConstraint;
  api.SetConstraintMotor = APISetConstraintMotor;
  return api;
}

}  // namespace Levye
