
#include "PhysicsService.hpp"

#include <Levye/Debug/Logger.hpp>

#ifdef LEVYE_PHYSICS_JOLT
#include <Levye/Modules/Physics/Backends/Jolt/JoltBackend.hpp>
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

namespace Levye {
namespace {

std::unique_ptr<IPhysicsBackend> CreateBackend(PhysicsBackendType type) {
  switch (type) {
    case PhysicsBackendType::Jolt:
#ifdef LEVYE_PHYSICS_JOLT
      return std::make_unique<JoltBackend>();
#else
      return nullptr;
#endif

    case PhysicsBackendType::Bullet:
    case PhysicsBackendType::PhysX:
      return nullptr;
  }

  return nullptr;
}

std::uint32_t NextGeneration(std::uint32_t generation) {
  ++generation;
  return generation == 0 ? 1 : generation;
}

}  // namespace

PhysicsService::~PhysicsService() { Shutdown(); }

PhysicsService::WorldSlot* PhysicsService::ResolveWorld(PhysicsWorldHandle world) {
  if (!world.IsValid() || world.index >= m_Worlds.size()) {
    return nullptr;
  }

  auto& slot = m_Worlds[world.index];

  if (!slot.backend || slot.generation != world.generation) {
    return nullptr;
  }

  return &slot;
}

const PhysicsService::WorldSlot* PhysicsService::ResolveWorld(PhysicsWorldHandle world) const {
  if (!world.IsValid() || world.index >= m_Worlds.size()) {
    return nullptr;
  }

  const auto& slot = m_Worlds[world.index];

  if (!slot.backend || slot.generation != world.generation) {
    return nullptr;
  }

  return &slot;
}

PhysicsWorldHandle PhysicsService::CreateWorld(const PhysicsWorldDesc& desc) {
  auto backend = CreateBackend(desc.backend);

  if (!backend || !backend->Initialize(desc)) {
    Logger::Warning("Physics world creation rejected: unsupported backend or invalid settings.");
    return {};
  }

  std::uint32_t index;

  if (!m_FreeWorlds.empty()) {
    index = m_FreeWorlds.back();
    m_FreeWorlds.pop_back();
  } else {
    if (m_Worlds.size() >= std::numeric_limits<std::uint32_t>::max()) {
      backend->Shutdown();
      return {};
    }

    index = static_cast<std::uint32_t>(m_Worlds.size());
    m_Worlds.emplace_back();
  }

  auto& slot = m_Worlds[index];
  slot.backend = std::move(backend);
  slot.paused = false;
  slot.pendingSingleSteps = 0;
  slot.completedSteps = 0;
  slot.rejectedCreations = 0;
  slot.lastStepMilliseconds = 0.0;

  return {index, slot.generation};
}

bool PhysicsService::DestroyWorld(PhysicsWorldHandle world) {
  auto* slot = ResolveWorld(world);

  if (!slot) return false;

  slot->bodies.clear();
  slot->characters.clear();
  slot->constraints.clear();
  slot->publicByBackend.clear();
  slot->contactEvents.Clear();
  slot->backend->Shutdown();
  slot->backend.reset();
  slot->generation = NextGeneration(slot->generation);

  m_FreeWorlds.push_back(world.index);

  return true;
}

bool PhysicsService::SetWorldPaused(PhysicsWorldHandle world, bool paused) {
  auto* slot = ResolveWorld(world);
  if (!slot) return false;
  slot->paused = paused;
  if (!paused) slot->pendingSingleSteps = 0;
  return true;
}

bool PhysicsService::StepWorldOnce(PhysicsWorldHandle world) {
  auto* slot = ResolveWorld(world);
  if (!slot || !slot->paused ||
      slot->pendingSingleSteps == std::numeric_limits<std::uint32_t>::max())
    return false;
  ++slot->pendingSingleSteps;
  return true;
}

bool PhysicsService::GetWorldDiagnostics(PhysicsWorldHandle world,
                                         PhysicsWorldDiagnostics& output) const {
  const auto* slot = ResolveWorld(world);
  if (!slot) return false;
  output = {};
  output.bodyCount = static_cast<std::uint32_t>(slot->bodies.size());
  output.characterCount = static_cast<std::uint32_t>(slot->characters.size());
  output.constraintCount = static_cast<std::uint32_t>(slot->constraints.size());
  output.activeContactPairs = slot->backend->ActiveContactPairCount();
  output.pendingContactEvents = static_cast<std::uint32_t>(slot->contactEvents.PendingCount());
  output.completedSteps = slot->completedSteps;
  output.rejectedCreations = slot->rejectedCreations;
  output.lastStepMilliseconds = slot->lastStepMilliseconds;
  output.paused = slot->paused;
  return true;
}

PhysicsBodyHandle PhysicsService::CreateBody(PhysicsWorldHandle world,
                                             const PhysicsBodyDesc& desc) {
  auto* slot = ResolveWorld(world);
  if (!slot) return {};

  const PhysicsBodyHandle backendHandle = slot->backend->CreateBody(desc);
  if (!backendHandle.IsValid()) ++slot->rejectedCreations;

  const PhysicsBodyHandle publicHandle = RegisterBody(*slot, backendHandle);

  if (!publicHandle.IsValid() && backendHandle.IsValid()) {
    if (!slot->backend->DestroyBody(backendHandle))
      Logger::Warning("Physics body cleanup failed after public handle registration failed.");
  }

  return publicHandle;
}

bool PhysicsService::DestroyBody(PhysicsWorldHandle world, PhysicsBodyHandle body) {
  auto* slot = ResolveWorld(world);
  if (!slot) return false;

  const auto it = slot->bodies.find(BodyKey(body));
  if (it == slot->bodies.end()) return false;

  const auto backendHandle = it->second;
  for (auto constraint = slot->constraints.begin(); constraint != slot->constraints.end();) {
    if (constraint->second.bodyA == body || constraint->second.bodyB == body) {
      if (!slot->backend->DestroyConstraint(constraint->second.backend)) return false;
      constraint = slot->constraints.erase(constraint);
    } else {
      ++constraint;
    }
  }
  DrainBackendContacts(*slot);
  if (!slot->backend->DestroyBody(backendHandle)) return false;
  DrainBackendContacts(*slot);
  slot->publicByBackend.erase(BodyKey(backendHandle));
  slot->bodies.erase(it);
  return true;
}

bool PhysicsService::GetTransform(PhysicsWorldHandle world, PhysicsBodyHandle body,
                                  Transform& output) const {
  auto* slot = ResolveWorld(world);
  if (!slot) return false;

  const auto* backendHandle = ResolveBody(*slot, body);
  if (!backendHandle) return false;

  return slot->backend->GetTransform(*backendHandle, output);
}

bool PhysicsService::SetTransform(PhysicsWorldHandle world, PhysicsBodyHandle body,
                                  const Transform& transform) {
  auto* slot = ResolveWorld(world);
  if (!slot) return false;

  const auto* backendHandle = ResolveBody(*slot, body);
  if (!backendHandle) return false;

  return slot->backend->SetTransform(*backendHandle, transform);
}

bool PhysicsService::SetLinearVelocity(PhysicsWorldHandle world, PhysicsBodyHandle body,
                                       Vector3 velocity) {
  auto* slot = ResolveWorld(world);
  if (!slot) return false;

  const auto* backendHandle = ResolveBody(*slot, body);
  if (!backendHandle) return false;

  return slot->backend->SetLinearVelocity(*backendHandle, velocity);
}

bool PhysicsService::GetLinearVelocity(PhysicsWorldHandle world, PhysicsBodyHandle body,
                                       Vector3& output) const {
  auto* slot = ResolveWorld(world);
  if (!slot) return false;

  const auto* backendHandle = ResolveBody(*slot, body);
  if (!backendHandle) return false;

  return slot->backend->GetLinearVelocity(*backendHandle, output);
}

bool PhysicsService::AddForce(PhysicsWorldHandle world, PhysicsBodyHandle body, Vector3 force) {
  auto* slot = ResolveWorld(world);
  if (!slot) return false;

  const auto* backendHandle = ResolveBody(*slot, body);
  if (!backendHandle) return false;

  return slot->backend->AddForce(*backendHandle, force);
}

bool PhysicsService::AddImpulse(PhysicsWorldHandle world, PhysicsBodyHandle body, Vector3 impulse) {
  auto* slot = ResolveWorld(world);
  if (!slot) return false;

  const auto* backendHandle = ResolveBody(*slot, body);
  if (!backendHandle) return false;

  return slot->backend->AddImpulse(*backendHandle, impulse);
}

bool PhysicsService::Raycast(PhysicsWorldHandle world, Vector3 origin, Vector3 direction,
                             float maxDistance, PhysicsRaycastHit& output) const {
  const auto* slot = ResolveWorld(world);
  if (!slot) return false;

  PhysicsRaycastHit backendHit{};

  if (!slot->backend->Raycast(origin, direction, maxDistance, backendHit)) {
    return false;
  }

  const PhysicsBodyHandle publicBody = FindPublicBody(*slot, backendHit.body);

  if (!publicBody.IsValid()) return false;

  backendHit.body = publicBody;
  output = backendHit;
  return true;
}

bool PhysicsService::RaycastFiltered(PhysicsWorldHandle world, Vector3 origin, Vector3 direction,
                                     float maxDistance, const PhysicsRaycastFilter& filter,
                                     PhysicsRaycastHit& output) const {
  const auto* slot = ResolveWorld(world);
  if (!slot) return false;

  PhysicsRaycastHit backendHit{};

  if (!slot->backend->RaycastFiltered(origin, direction, maxDistance, filter, backendHit)) {
    return false;
  }

  const PhysicsBodyHandle publicBody = FindPublicBody(*slot, backendHit.body);

  if (!publicBody.IsValid()) return false;

  backendHit.body = publicBody;
  output = backendHit;
  return true;
}

bool PhysicsService::SetCollisionFilter(PhysicsWorldHandle world, PhysicsBodyHandle body,
                                        std::uint16_t layer, std::uint16_t mask) {
  auto* slot = ResolveWorld(world);
  if (!slot) return false;

  const auto* backendHandle = ResolveBody(*slot, body);
  if (!backendHandle) return false;

  DrainBackendContacts(*slot);

  if (!slot->backend->SetCollisionFilter(*backendHandle, layer, mask)) {
    return false;
  }

  DrainBackendContacts(*slot);
  return true;
}

bool PhysicsService::ShapeCast(PhysicsWorldHandle world, const PhysicsShapeDesc& shape,
                               const Transform& start, Vector3 direction, float maxDistance,
                               const PhysicsQueryFilter& filter,
                               PhysicsShapeCastHit& output) const {
  const auto* slot = ResolveWorld(world);
  if (!slot) return false;
  PhysicsQueryFilter backendFilter = filter;
  if (filter.ignoredBody.IsValid()) {
    const auto* ignored = ResolveBody(*slot, filter.ignoredBody);
    if (!ignored) return false;
    backendFilter.ignoredBody = *ignored;
  }
  PhysicsShapeCastHit hit{};
  if (!slot->backend->ShapeCast(shape, start, direction, maxDistance, backendFilter, hit))
    return false;
  hit.body = FindPublicBody(*slot, hit.body);
  if (!hit.body.IsValid()) return false;
  output = hit;
  return true;
}

bool PhysicsService::OverlapShape(PhysicsWorldHandle world, const PhysicsShapeDesc& shape,
                                  const Transform& transform, const PhysicsQueryFilter& filter,
                                  PhysicsBodyHandle* results, std::uint32_t capacity,
                                  std::uint32_t& count) const {
  count = 0;
  const auto* slot = ResolveWorld(world);
  if (!slot || (capacity != 0 && !results)) return false;
  PhysicsQueryFilter backendFilter = filter;
  if (filter.ignoredBody.IsValid()) {
    const auto* ignored = ResolveBody(*slot, filter.ignoredBody);
    if (!ignored) return false;
    backendFilter.ignoredBody = *ignored;
  }
  std::vector<PhysicsBodyHandle> hits;
  try {
    hits.resize(slot->bodies.size());
  } catch (const std::bad_alloc&) {
    return false;
  }
  const auto available = static_cast<std::uint32_t>(hits.size());
  if (!slot->backend->OverlapShape(shape, transform, backendFilter, hits.data(), available, count))
    return false;
  if (count > available) return false;
  for (std::uint32_t i = 0; i < count; ++i) {
    hits[i] = FindPublicBody(*slot, hits[i]);
    if (!hits[i].IsValid()) return false;
  }
  std::sort(hits.begin(), hits.begin() + count, [](auto a, auto b) {
    return a.index < b.index || (a.index == b.index && a.generation < b.generation);
  });
  const auto written = std::min(count, capacity);
  for (std::uint32_t i = 0; i < written; ++i) results[i] = hits[i];
  return true;
}

PhysicsCharacterHandle PhysicsService::CreateCharacter(PhysicsWorldHandle world,
                                                       const PhysicsCharacterDesc& desc) {
  auto* slot = ResolveWorld(world);
  if (!slot || m_NextCharacterId >= std::numeric_limits<std::uint32_t>::max()) return {};
  const auto backend = slot->backend->CreateCharacter(desc);
  if (!backend.IsValid()) {
    ++slot->rejectedCreations;
    return {};
  }
  const PhysicsCharacterHandle result{static_cast<std::uint32_t>(m_NextCharacterId++), 1};
  try {
    if (slot->characters.emplace(BodyKey({result.index, result.generation}), backend).second)
      return result;
  } catch (const std::bad_alloc&) {
  }
  if (!slot->backend->DestroyCharacter(backend))
    Logger::Warning("Physics character cleanup failed after public handle registration failed.");
  return {};
}

const PhysicsCharacterHandle* PhysicsService::ResolveCharacter(
    const WorldSlot& slot, PhysicsCharacterHandle publicHandle) const {
  if (!publicHandle.IsValid()) return nullptr;
  const auto it = slot.characters.find(BodyKey({publicHandle.index, publicHandle.generation}));
  return it == slot.characters.end() ? nullptr : &it->second;
}

bool PhysicsService::DestroyCharacter(PhysicsWorldHandle world, PhysicsCharacterHandle character) {
  auto* slot = ResolveWorld(world);
  if (!slot) return false;
  const auto* backend = ResolveCharacter(*slot, character);
  if (!backend || !slot->backend->DestroyCharacter(*backend)) return false;
  slot->characters.erase(BodyKey({character.index, character.generation}));
  return true;
}

bool PhysicsService::SetCharacterVelocity(PhysicsWorldHandle world,
                                          PhysicsCharacterHandle character, Vector3 horizontal) {
  auto* slot = ResolveWorld(world);
  const auto* backend = slot ? ResolveCharacter(*slot, character) : nullptr;
  return backend && slot->backend->SetCharacterVelocity(*backend, horizontal);
}

bool PhysicsService::JumpCharacter(PhysicsWorldHandle world, PhysicsCharacterHandle character,
                                   float speed) {
  auto* slot = ResolveWorld(world);
  const auto* backend = slot ? ResolveCharacter(*slot, character) : nullptr;
  return backend && slot->backend->JumpCharacter(*backend, speed);
}

bool PhysicsService::SetCharacterPosition(PhysicsWorldHandle world,
                                          PhysicsCharacterHandle character, Vector3 position) {
  auto* slot = ResolveWorld(world);
  const auto* backend = slot ? ResolveCharacter(*slot, character) : nullptr;
  return backend && slot->backend->SetCharacterPosition(*backend, position);
}

bool PhysicsService::GetCharacterPosition(PhysicsWorldHandle world,
                                          PhysicsCharacterHandle character, Vector3& output) const {
  const auto* slot = ResolveWorld(world);
  const auto* backend = slot ? ResolveCharacter(*slot, character) : nullptr;
  return backend && slot->backend->GetCharacterPosition(*backend, output);
}

bool PhysicsService::GetCharacterVelocity(PhysicsWorldHandle world,
                                          PhysicsCharacterHandle character, Vector3& output) const {
  const auto* slot = ResolveWorld(world);
  const auto* backend = slot ? ResolveCharacter(*slot, character) : nullptr;
  return backend && slot->backend->GetCharacterVelocity(*backend, output);
}

bool PhysicsService::GetCharacterGroundState(PhysicsWorldHandle world,
                                             PhysicsCharacterHandle character,
                                             PhysicsGroundState& output) const {
  const auto* slot = ResolveWorld(world);
  const auto* backend = slot ? ResolveCharacter(*slot, character) : nullptr;
  return backend && slot->backend->GetCharacterGroundState(*backend, output);
}

bool PhysicsService::GetCharacterGroundNormal(PhysicsWorldHandle world,
                                              PhysicsCharacterHandle character,
                                              Vector3& output) const {
  const auto* slot = ResolveWorld(world);
  const auto* backend = slot ? ResolveCharacter(*slot, character) : nullptr;
  return backend && slot->backend->GetCharacterGroundNormal(*backend, output);
}

PhysicsConstraintHandle PhysicsService::CreateConstraint(PhysicsWorldHandle world,
                                                         const PhysicsConstraintDesc& desc) {
  auto* slot = ResolveWorld(world);
  if (!slot || m_NextConstraintId >= std::numeric_limits<std::uint32_t>::max()) return {};
  const auto* bodyA = ResolveBody(*slot, desc.bodyA);
  const auto* bodyB = ResolveBody(*slot, desc.bodyB);
  if (!bodyA || !bodyB || desc.bodyA == desc.bodyB) return {};
  PhysicsConstraintDesc backendDesc = desc;
  backendDesc.bodyA = *bodyA;
  backendDesc.bodyB = *bodyB;
  const auto backend = slot->backend->CreateConstraint(backendDesc);
  if (!backend.IsValid()) {
    ++slot->rejectedCreations;
    return {};
  }
  const PhysicsConstraintHandle result{static_cast<std::uint32_t>(m_NextConstraintId++), 1};
  try {
    if (slot->constraints
            .emplace(BodyKey({result.index, result.generation}),
                     WorldSlot::ConstraintRecord{backend, desc.bodyA, desc.bodyB})
            .second)
      return result;
  } catch (const std::bad_alloc&) {
  }
  (void)slot->backend->DestroyConstraint(backend);
  return {};
}

bool PhysicsService::DestroyConstraint(PhysicsWorldHandle world,
                                       PhysicsConstraintHandle constraint) {
  auto* slot = ResolveWorld(world);
  if (!slot) return false;
  const auto it = slot->constraints.find(BodyKey({constraint.index, constraint.generation}));
  if (it == slot->constraints.end() || !slot->backend->DestroyConstraint(it->second.backend))
    return false;
  slot->constraints.erase(it);
  return true;
}

bool PhysicsService::SetConstraintMotor(PhysicsWorldHandle world,
                                        PhysicsConstraintHandle constraint,
                                        const PhysicsMotorDesc& motor) {
  auto* slot = ResolveWorld(world);
  if (!slot) return false;
  const auto it = slot->constraints.find(BodyKey({constraint.index, constraint.generation}));
  return it != slot->constraints.end() &&
         slot->backend->SetConstraintMotor(it->second.backend, motor);
}

void PhysicsService::Step(float fixedDeltaTime) {
  if (!std::isfinite(fixedDeltaTime) || fixedDeltaTime <= 0.0f) {
    return;
  }

  for (auto& slot : m_Worlds) {
    if (slot.backend) {
      if (slot.paused && slot.pendingSingleSteps == 0) continue;
      if (slot.paused) --slot.pendingSingleSteps;
      const auto start = std::chrono::steady_clock::now();
      slot.backend->Step(fixedDeltaTime);
      DrainBackendContacts(slot);
      const auto end = std::chrono::steady_clock::now();
      slot.lastStepMilliseconds = std::chrono::duration<double, std::milli>(end - start).count();
      ++slot.completedSteps;
    }
  }
}

void PhysicsService::Shutdown() {
  for (auto& slot : m_Worlds) {
    slot.bodies.clear();
    slot.characters.clear();
    slot.constraints.clear();
    slot.publicByBackend.clear();
    slot.contactEvents.Clear();

    if (slot.backend) {
      slot.backend->Shutdown();
      slot.backend.reset();
    }

    slot.generation = NextGeneration(slot.generation);
  }

  m_FreeWorlds.clear();

  for (std::uint32_t i = 0; i < m_Worlds.size(); ++i) {
    m_FreeWorlds.push_back(i);
  }
}

std::uint64_t PhysicsService::BodyKey(PhysicsBodyHandle handle) {
  return (static_cast<std::uint64_t>(handle.generation) << 32) | handle.index;
}

PhysicsBodyHandle PhysicsService::RegisterBody(WorldSlot& slot, PhysicsBodyHandle backendHandle) {
  if (!backendHandle.IsValid()) return {};

  if (m_NextBodyId == 0 || m_NextBodyId >= std::numeric_limits<std::uint32_t>::max()) {
    return {};
  }

  PhysicsBodyHandle publicHandle{static_cast<std::uint32_t>(m_NextBodyId++), 1};

  try {
    const auto [it, inserted] = slot.bodies.emplace(BodyKey(publicHandle), backendHandle);
    if (!inserted) return {};
    try {
      if (!slot.publicByBackend.emplace(BodyKey(backendHandle), publicHandle).second) {
        slot.bodies.erase(it);
        return {};
      }
    } catch (const std::bad_alloc&) {
      slot.bodies.erase(it);
      return {};
    }
  } catch (const std::bad_alloc&) {
    return {};
  }
  return publicHandle;
}

const PhysicsBodyHandle* PhysicsService::ResolveBody(const WorldSlot& slot,
                                                     PhysicsBodyHandle publicHandle) const {
  if (!publicHandle.IsValid()) return nullptr;

  const auto it = slot.bodies.find(BodyKey(publicHandle));
  return it == slot.bodies.end() ? nullptr : &it->second;
}

PhysicsBodyHandle PhysicsService::FindPublicBody(const WorldSlot& slot,
                                                 PhysicsBodyHandle backendHandle) const {
  const auto it = slot.publicByBackend.find(BodyKey(backendHandle));
  return it == slot.publicByBackend.end() ? PhysicsBodyHandle{} : it->second;
}

void PhysicsService::DrainBackendContacts(WorldSlot& slot) {
  PhysicsContactEvent event{};
  while (slot.backend->PollContactEvent(event)) {
    const auto a = FindPublicBody(slot, event.bodyA);
    const auto b = FindPublicBody(slot, event.bodyB);
    if (!a.IsValid() || !b.IsValid()) continue;
    event.bodyA = a;
    event.bodyB = b;
    slot.contactEvents.Push(event);
  }
  const auto overflow = slot.contactEvents.TakeOverflowCount();
  if (overflow != 0)
    Logger::Warning("Physics public contact queue overflow: " + std::to_string(overflow) +
                    " events coalesced; terminal contacts retained.");
}

bool PhysicsService::PollContactEvent(PhysicsWorldHandle world, PhysicsContactEvent& output) {
  auto* slot = ResolveWorld(world);
  return slot && slot->contactEvents.Poll(output);
}

}  // namespace Levye
