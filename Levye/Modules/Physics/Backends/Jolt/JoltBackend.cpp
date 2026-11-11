#include <Jolt/Jolt.h>
//

#include <Levye/Debug/Logger.hpp>
#include <Levye/Modules/Physics/ContactPairRegistry.hpp>

#include "JoltBackend.hpp"
//
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/RegisterTypes.h>
//
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
//
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/PhysicsMaterial.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/ShapeCast.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Levye {

namespace {

// ---------------------------------------------------------
// Collision layers
// ---------------------------------------------------------
namespace BroadPhaseLayers {
constexpr JPH::BroadPhaseLayer Static{0};
constexpr JPH::BroadPhaseLayer Moving{1};
constexpr JPH::uint Count = 2;
}  // namespace BroadPhaseLayers

struct CollisionFilter {
  std::uint16_t layer = 1;
  std::uint16_t mask = 0xFFFF;
  bool isStatic = false;

  bool operator==(const CollisionFilter&) const = default;
};

class CollisionLayerRegistry {
 public:
  JPH::ObjectLayer Register(const CollisionFilter& filter) {
    for (std::size_t i = 0; i < filters_.size(); ++i) {
      if (filters_[i] == filter) {
        return static_cast<JPH::ObjectLayer>(i);
      }
    }

    constexpr std::size_t maxLayers =
        static_cast<std::size_t>(std::numeric_limits<JPH::ObjectLayer>::max()) + 1;

    if (filters_.size() >= maxLayers) {
      throw std::runtime_error("Jolt object-layer registry exhausted");
    }

    const auto id = static_cast<JPH::ObjectLayer>(filters_.size());

    filters_.push_back(filter);
    return id;
  }

  const CollisionFilter& Get(JPH::ObjectLayer id) const {
    return filters_[static_cast<std::size_t>(id)];
  }

  void Clear() { filters_.clear(); }

 private:
  std::vector<CollisionFilter> filters_;
};

class LayerInterface final : public JPH::BroadPhaseLayerInterface {
 public:
  explicit LayerInterface(const CollisionLayerRegistry& registry) : registry_(registry) {}

  JPH::uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::Count; }

  JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override {
    return registry_.Get(layer).isStatic ? BroadPhaseLayers::Static : BroadPhaseLayers::Moving;
  }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
  const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override {
    return layer == BroadPhaseLayers::Static ? "Static" : "Moving";
  }
#endif

 private:
  const CollisionLayerRegistry& registry_;
};

class ObjectLayerFilter final : public JPH::ObjectLayerPairFilter {
 public:
  explicit ObjectLayerFilter(const CollisionLayerRegistry& registry) : registry_(registry) {}

  bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override {
    const auto& first = registry_.Get(a);
    const auto& second = registry_.Get(b);

    if (first.isStatic && second.isStatic) {
      return false;
    }

    return (first.mask & second.layer) != 0 && (second.mask & first.layer) != 0;
  }

 private:
  const CollisionLayerRegistry& registry_;
};

class BroadPhaseFilter final : public JPH::ObjectVsBroadPhaseLayerFilter {
 public:
  explicit BroadPhaseFilter(const CollisionLayerRegistry& registry) : registry_(registry) {}

  bool ShouldCollide(JPH::ObjectLayer objectLayer,
                     JPH::BroadPhaseLayer broadPhaseLayer) const override {
    const auto& filter = registry_.Get(objectLayer);

    // Static bodies never collide with other static bodies.
    if (filter.isStatic) {
      return broadPhaseLayer == BroadPhaseLayers::Moving;
    }

    return true;
  }

 private:
  const CollisionLayerRegistry& registry_;
};

// ---------------------------------------------------------
// Math conversions
// ---------------------------------------------------------

JPH::Vec3 ToJolt(Vector3 value) { return {value.x, value.y, value.z}; }

JPH::RVec3 ToJoltPosition(Vector3 value) { return {value.x, value.y, value.z}; }

JPH::Quat ToJolt(Quaternion value) { return {value.x, value.y, value.z, value.w}; }

Vector3 FromJolt(JPH::Vec3Arg value) { return {value.GetX(), value.GetY(), value.GetZ()}; }

Vector3 FromJoltPosition(JPH::RVec3Arg value) {
  return {static_cast<float>(value.GetX()), static_cast<float>(value.GetY()),
          static_cast<float>(value.GetZ())};
}

Quaternion FromJolt(JPH::QuatArg value) {
  return {value.GetX(), value.GetY(), value.GetZ(), value.GetW()};
}

bool IsFinite(Vector3 value) {
  return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool IsValidRotation(Quaternion value) {
  const float lengthSquared =
      value.x * value.x + value.y * value.y + value.z * value.z + value.w * value.w;

  return std::isfinite(lengthSquared) && lengthSquared > 0.000001f;
}

JPH::Quat ToNormalizedJolt(Quaternion value) {
  const float length =
      std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z + value.w * value.w);

  return JPH::Quat(value.x / length, value.y / length, value.z / length, value.w / length);
}

// ---------------------------------------------------------
// Process-wide Jolt lifecycle
// ---------------------------------------------------------

std::mutex gJoltMutex;
std::uint32_t gJoltUsers = 0;

void AcquireJolt() {
  std::lock_guard lock(gJoltMutex);

  if (gJoltUsers++ != 0) {
    return;
  }

  JPH::RegisterDefaultAllocator();

  JPH::Factory::sInstance = new JPH::Factory();

  JPH::RegisterTypes();
}

void ReleaseJolt() {
  std::lock_guard lock(gJoltMutex);

  if (gJoltUsers == 0 || --gJoltUsers != 0) {
    return;
  }

  JPH::UnregisterTypes();

  delete JPH::Factory::sInstance;
  JPH::Factory::sInstance = nullptr;
}

JPH::EMotionType ToMotionType(PhysicsMotionType type) {
  switch (type) {
    case PhysicsMotionType::Static:
      return JPH::EMotionType::Static;

    case PhysicsMotionType::Kinematic:
      return JPH::EMotionType::Kinematic;

    case PhysicsMotionType::Dynamic:
    default:
      return JPH::EMotionType::Dynamic;
  }
}

JPH::RefConst<JPH::Shape> CreateShape(const PhysicsShapeDesc& desc, bool allowMesh = false,
                                      unsigned int depth = 0) {
  if (depth > 4) return nullptr;
  switch (desc.type) {
    case PhysicsShapeType::Box:
      if (!IsFinite(desc.halfExtents) || desc.halfExtents.x <= 0.0f || desc.halfExtents.y <= 0.0f ||
          desc.halfExtents.z <= 0.0f) {
        return nullptr;
      }

      return new JPH::BoxShape(ToJolt(desc.halfExtents));

    case PhysicsShapeType::Sphere:
      if (!std::isfinite(desc.radius) || desc.radius <= 0.0f) {
        return nullptr;
      }

      return new JPH::SphereShape(desc.radius);

    case PhysicsShapeType::Capsule:
      if (!std::isfinite(desc.radius) || !std::isfinite(desc.halfHeight) || desc.radius <= 0.0f ||
          desc.halfHeight <= 0.0f) {
        return nullptr;
      }

      return new JPH::CapsuleShape(desc.halfHeight, desc.radius);

    case PhysicsShapeType::ConvexHull: {
      if (!desc.vertices || desc.vertexCount < 4 || desc.vertexCount > 256) return nullptr;
      JPH::Array<JPH::Vec3> points;
      points.reserve(desc.vertexCount);
      for (std::uint32_t i = 0; i < desc.vertexCount; ++i) {
        if (!IsFinite(desc.vertices[i])) return nullptr;
        points.push_back(ToJolt(desc.vertices[i]));
      }
      JPH::ConvexHullShapeSettings settings(points);
      const auto result = settings.Create();
      return result.IsValid() ? result.Get() : nullptr;
    }

    case PhysicsShapeType::TriangleMesh: {
      if (!allowMesh || !desc.vertices || !desc.indices || desc.vertexCount < 3 ||
          desc.vertexCount > 65536 || desc.indexCount < 3 || desc.indexCount > 196608 ||
          desc.indexCount % 3 != 0)
        return nullptr;
      JPH::VertexList vertices;
      vertices.reserve(desc.vertexCount);
      for (std::uint32_t i = 0; i < desc.vertexCount; ++i) {
        const auto point = desc.vertices[i];
        if (!IsFinite(point)) return nullptr;
        vertices.emplace_back(point.x, point.y, point.z);
      }
      JPH::IndexedTriangleList triangles;
      triangles.reserve(desc.indexCount / 3);
      for (std::uint32_t i = 0; i < desc.indexCount; i += 3) {
        if (desc.indices[i] >= desc.vertexCount || desc.indices[i + 1] >= desc.vertexCount ||
            desc.indices[i + 2] >= desc.vertexCount)
          return nullptr;
        triangles.emplace_back(desc.indices[i], desc.indices[i + 1], desc.indices[i + 2]);
      }
      JPH::MeshShapeSettings settings(std::move(vertices), std::move(triangles));
      const auto result = settings.Create();
      return result.IsValid() ? result.Get() : nullptr;
    }

    case PhysicsShapeType::Compound: {
      if (!desc.children || desc.childCount == 0 || desc.childCount > 32) return nullptr;
      JPH::StaticCompoundShapeSettings settings;
      for (std::uint32_t i = 0; i < desc.childCount; ++i) {
        const auto& child = desc.children[i];
        const auto& transform = child.localTransform;
        if (!child.shape || !IsFinite(transform.translation) ||
            !IsValidRotation(transform.rotation) || !IsFinite(transform.scale) ||
            std::abs(transform.scale.x - 1.0f) > 0.0001f ||
            std::abs(transform.scale.y - 1.0f) > 0.0001f ||
            std::abs(transform.scale.z - 1.0f) > 0.0001f)
          return nullptr;
        const auto shape = CreateShape(*child.shape, allowMesh, depth + 1);
        if (!shape) return nullptr;
        settings.AddShape(ToJolt(transform.translation), ToNormalizedJolt(transform.rotation),
                          shape.GetPtr());
      }
      const auto result = settings.Create();
      return result.IsValid() ? result.Get() : nullptr;
    }
  }

  return nullptr;
}

class ContactEventListener final : public JPH::ContactListener {
 public:
  JPH::ValidateResult OnContactValidate(const JPH::Body& body1, const JPH::Body& body2,
                                        JPH::RVec3Arg, const JPH::CollideShapeResult&) override {
    const auto pair = Pair(body1.GetID(), body2.GetID());
    std::lock_guard lock(suppressedMutex_);
    return suppressedPairs_.contains(pair) ? JPH::ValidateResult::RejectAllContactsForThisBodyPair
                                           : JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
  }

  void Suppress(JPH::BodyID first, JPH::BodyID second) {
    std::lock_guard lock(suppressedMutex_);
    ++suppressedPairs_[Pair(first, second)];
  }

  void Unsuppress(JPH::BodyID first, JPH::BodyID second) {
    std::lock_guard lock(suppressedMutex_);
    const auto pair = Pair(first, second);
    const auto it = suppressedPairs_.find(pair);
    if (it != suppressedPairs_.end() && --it->second == 0) suppressedPairs_.erase(it);
  }

  void OnContactAdded(const JPH::Body& body1, const JPH::Body& body2,
                      const JPH::ContactManifold& manifold, JPH::ContactSettings&) override {
    Record(ContactObservation::Kind::Added, body1, body2, manifold);
  }

  void OnContactPersisted(const JPH::Body& body1, const JPH::Body& body2,
                          const JPH::ContactManifold& manifold, JPH::ContactSettings&) override {
    Record(ContactObservation::Kind::Persisted, body1, body2, manifold);
  }

  void OnContactRemoved(const JPH::SubShapeIDPair& pair) override {
    ContactObservation observation{};
    observation.kind = ContactObservation::Kind::Removed;
    observation.bodyA = pair.GetBody1ID().GetIndexAndSequenceNumber();
    observation.bodyB = pair.GetBody2ID().GetIndexAndSequenceNumber();
    observation.shapeA = pair.GetSubShapeID1().GetValue();
    observation.shapeB = pair.GetSubShapeID2().GetValue();
    std::lock_guard lock(mutex_);
    observations_.push_back(observation);
  }

  void Take(std::vector<ContactObservation>& output) {
    std::lock_guard lock(mutex_);
    output.clear();
    output.swap(observations_);
  }

  void Clear() {
    std::lock_guard lock(mutex_);
    observations_.clear();
    std::lock_guard suppressedLock(suppressedMutex_);
    suppressedPairs_.clear();
  }

 private:
  using BodyPair = std::pair<std::uint32_t, std::uint32_t>;
  static BodyPair Pair(JPH::BodyID first, JPH::BodyID second) {
    const auto a = first.GetIndexAndSequenceNumber();
    const auto b = second.GetIndexAndSequenceNumber();
    return std::minmax(a, b);
  }
  void Record(ContactObservation::Kind kind, const JPH::Body& body1, const JPH::Body& body2,
              const JPH::ContactManifold& manifold) {
    ContactObservation observation{};
    observation.kind = kind;
    observation.bodyA = body1.GetID().GetIndexAndSequenceNumber();
    observation.bodyB = body2.GetID().GetIndexAndSequenceNumber();
    observation.shapeA = manifold.mSubShapeID1.GetValue();
    observation.shapeB = manifold.mSubShapeID2.GetValue();
    observation.position = FromJoltPosition(manifold.mBaseOffset);
    observation.normal = FromJolt(manifold.mWorldSpaceNormal);
    observation.isSensor = body1.IsSensor() || body2.IsSensor();
    std::lock_guard lock(mutex_);
    observations_.push_back(observation);
  }

  std::mutex mutex_;
  std::vector<ContactObservation> observations_;
  std::mutex suppressedMutex_;
  std::map<BodyPair, std::uint32_t> suppressedPairs_;
};

class RaycastObjectLayerFilter final : public JPH::ObjectLayerFilter {
 public:
  RaycastObjectLayerFilter(const CollisionLayerRegistry& registry, std::uint16_t mask)
      : registry_(registry), mask_(mask) {}

  bool ShouldCollide(JPH::ObjectLayer layer) const override {
    return (registry_.Get(layer).layer & mask_) != 0;
  }

 private:
  const CollisionLayerRegistry& registry_;
  std::uint16_t mask_;
};

class RaycastSensorFilter final : public JPH::BodyFilter {
 public:
  explicit RaycastSensorFilter(bool includeSensors) : includeSensors_(includeSensors) {}

  bool ShouldCollide(const JPH::BodyID&) const override { return true; }

  bool ShouldCollideLocked(const JPH::Body& body) const override {
    return includeSensors_ || !body.IsSensor();
  }

 private:
  bool includeSensors_;
};

class QueryBodyFilter final : public JPH::BodyFilter {
 public:
  QueryBodyFilter(bool includeSensors, JPH::BodyID ignored)
      : includeSensors_(includeSensors), ignored_(ignored) {}
  bool ShouldCollide(const JPH::BodyID& id) const override { return id != ignored_; }
  bool ShouldCollideLocked(const JPH::Body& body) const override {
    return body.GetID() != ignored_ && (includeSensors_ || !body.IsSensor());
  }

 private:
  bool includeSensors_;
  JPH::BodyID ignored_;
};

class CharacterObjectFilter final : public JPH::ObjectLayerFilter {
 public:
  CharacterObjectFilter(const CollisionLayerRegistry& registry, std::uint16_t layer,
                        std::uint16_t mask)
      : registry_(registry), layer_(layer), mask_(mask) {}
  bool ShouldCollide(JPH::ObjectLayer objectLayer) const override {
    const auto& other = registry_.Get(objectLayer);
    return (mask_ & other.layer) != 0 && (other.mask & layer_) != 0;
  }

 private:
  const CollisionLayerRegistry& registry_;
  std::uint16_t layer_;
  std::uint16_t mask_;
};

bool ValidQueryTransform(const Transform& transform) {
  if (!IsFinite(transform.translation) || !IsValidRotation(transform.rotation)) return false;
  return std::isfinite(transform.scale.x) && std::isfinite(transform.scale.y) &&
         std::isfinite(transform.scale.z) && std::abs(transform.scale.x - 1.0f) <= 0.0001f &&
         std::abs(transform.scale.y - 1.0f) <= 0.0001f &&
         std::abs(transform.scale.z - 1.0f) <= 0.0001f;
}

bool ValidMotor(const PhysicsMotorDesc& motor) {
  return (motor.mode == PhysicsMotorMode::Off || motor.mode == PhysicsMotorMode::Velocity ||
          motor.mode == PhysicsMotorMode::Position) &&
         std::isfinite(motor.target) && std::isfinite(motor.maxForce) &&
         std::isfinite(motor.frequency) && std::isfinite(motor.damping) && motor.maxForce > 0 &&
         motor.frequency >= 0 && motor.damping >= 0;
}

bool ConfigureMotor(JPH::TwoBodyConstraint* constraint, PhysicsConstraintType type,
                    const PhysicsMotorDesc& motor) {
  if (!ValidMotor(motor)) return false;
  const auto state = motor.mode == PhysicsMotorMode::Velocity   ? JPH::EMotorState::Velocity
                     : motor.mode == PhysicsMotorMode::Position ? JPH::EMotorState::Position
                                                                : JPH::EMotorState::Off;
  if (type == PhysicsConstraintType::Hinge) {
    auto* hinge = static_cast<JPH::HingeConstraint*>(constraint);
    hinge->GetMotorSettings() = JPH::MotorSettings(motor.frequency, motor.damping);
    hinge->GetMotorSettings().SetTorqueLimit(motor.maxForce);
    hinge->SetTargetAngularVelocity(motor.target);
    hinge->SetTargetAngle(motor.target);
    hinge->SetMotorState(state);
    return true;
  }
  if (type == PhysicsConstraintType::Slider) {
    auto* slider = static_cast<JPH::SliderConstraint*>(constraint);
    slider->GetMotorSettings() = JPH::MotorSettings(motor.frequency, motor.damping);
    slider->GetMotorSettings().SetForceLimit(motor.maxForce);
    slider->SetTargetVelocity(motor.target);
    slider->SetTargetPosition(motor.target);
    slider->SetMotorState(state);
    return true;
  }
  return false;
}

}  // namespace

// ---------------------------------------------------------
// Jolt backend implementation state
// ---------------------------------------------------------

struct JoltBackend::Impl {
  struct BodySlot {
    JPH::BodyID id{};
    std::uint32_t generation = 1;
    bool occupied = false;

    std::uint16_t layer = 1;
    std::uint16_t mask = 0xFFFF;
    bool isStatic = false;
  };

  struct CharacterSlot {
    JPH::Ref<JPH::CharacterVirtual> character;
    std::uint32_t generation = 1;
    PhysicsCharacterDesc desc{};
    Vector3 desiredHorizontal{};
    Vector3 actualVelocity{};
    // Platform velocity retained until landing after a jump.
    Vector3 airborneCarry{};
    float verticalSpeed = 0;
    bool jumping = false;
  };

  struct ConstraintSlot {
    JPH::Ref<JPH::TwoBodyConstraint> constraint;
    JPH::BodyID bodyA{};
    JPH::BodyID bodyB{};
    PhysicsConstraintType type = PhysicsConstraintType::Fixed;
    std::uint32_t generation = 1;
    bool suppressCollision = false;
  };

  CollisionLayerRegistry collisionLayers;

  LayerInterface broadPhaseLayers{collisionLayers};
  ObjectLayerFilter objectLayerFilter{collisionLayers};
  BroadPhaseFilter broadPhaseFilter{collisionLayers};
  ContactEventListener contactListener;
  std::vector<ContactObservation> observationScratch;
  ContactPairRegistry contactPairs;
  ContactEventQueue contactEvents;

  std::unique_ptr<JPH::PhysicsSystem> physics;
  std::unique_ptr<JPH::TempAllocatorImpl> allocator;
  std::unique_ptr<JPH::JobSystemThreadPool> jobs;

  std::vector<BodySlot> bodies;
  std::vector<std::uint32_t> freeSlots;
  std::unordered_map<std::uint32_t, PhysicsBodyHandle> bodyByID;
  std::vector<CharacterSlot> characters;
  std::vector<std::uint32_t> freeCharacters;
  std::vector<ConstraintSlot> constraints;
  std::vector<std::uint32_t> freeConstraints;
  // Shared by all virtual characters in this physics world.
  JPH::CharacterVsCharacterCollisionSimple characterCollisions;
  Vector3 gravity{};

  bool initialized = false;

  CharacterSlot* ResolveCharacter(PhysicsCharacterHandle handle) {
    if (!handle.IsValid() || handle.index >= characters.size()) return nullptr;
    auto& slot = characters[handle.index];
    return slot.character && slot.generation == handle.generation ? &slot : nullptr;
  }
  const CharacterSlot* ResolveCharacter(PhysicsCharacterHandle handle) const {
    return const_cast<Impl*>(this)->ResolveCharacter(handle);
  }

  ConstraintSlot* ResolveConstraint(PhysicsConstraintHandle handle) {
    if (!handle.IsValid() || handle.index >= constraints.size()) return nullptr;
    auto& slot = constraints[handle.index];
    return slot.constraint && slot.generation == handle.generation ? &slot : nullptr;
  }

  JPH::BodyID Resolve(PhysicsBodyHandle handle) const {
    if (!handle.IsValid() || handle.index >= bodies.size()) {
      return {};
    }

    const BodySlot& slot = bodies[handle.index];

    if (!slot.occupied || slot.generation != handle.generation) {
      return {};
    }

    return slot.id;
  }

  PhysicsBodyHandle Find(JPH::BodyID id) const {
    const auto it = bodyByID.find(id.GetIndexAndSequenceNumber());
    return it == bodyByID.end() ? PhysicsBodyHandle{} : it->second;
  }

  void Emit(const std::vector<ContactPairRegistry::Event>& events) {
    for (const auto& source : events) {
      const auto a = bodyByID.find(source.pair.first);
      const auto b = bodyByID.find(source.pair.second);
      if (a == bodyByID.end() || b == bodyByID.end()) continue;
      PhysicsContactEvent event{};
      event.type = source.type;
      event.bodyA = a->second;
      event.bodyB = b->second;
      event.position = source.position;
      event.normal = source.normal;
      event.isSensor = source.isSensor;
      contactEvents.Push(event);
    }
  }

  void ApplyObservations() {
    // Jolt can report removal of an old subshape after a filter change. Reject
    // stale Added/Persisted observations before they can restart that pair.
    observationScratch.erase(std::remove_if(observationScratch.begin(), observationScratch.end(),
                                            [&](const auto& o) {
                                              if (o.kind == ContactObservation::Kind::Removed)
                                                return false;
                                              const auto a = bodyByID.find(o.bodyA);
                                              const auto b = bodyByID.find(o.bodyB);
                                              if (a == bodyByID.end() || b == bodyByID.end())
                                                return true;
                                              const auto& first = bodies[a->second.index];
                                              const auto& second = bodies[b->second.index];
                                              return (first.isStatic && second.isStatic) ||
                                                     (first.mask & second.layer) == 0 ||
                                                     (second.mask & first.layer) == 0;
                                            }),
                             observationScratch.end());
    Emit(contactPairs.Apply(observationScratch));
  }

  PhysicsBodyHandle Store(JPH::BodyID id, std::uint16_t layer, std::uint16_t mask, bool isStatic) {
    const bool reuse = !freeSlots.empty();
    if (!reuse && bodies.size() >= std::numeric_limits<std::uint32_t>::max()) return {};
    const auto index = reuse ? freeSlots.back() : static_cast<std::uint32_t>(bodies.size());
    try {
      if (!reuse) bodies.emplace_back();
      const PhysicsBodyHandle handle{index, bodies[index].generation};
      const auto [_, inserted] = bodyByID.emplace(id.GetIndexAndSequenceNumber(), handle);
      if (!inserted) {
        if (!reuse) bodies.pop_back();
        return {};
      }
      if (reuse) freeSlots.pop_back();
      auto& slot = bodies[index];
      slot.id = id;
      slot.occupied = true;
      slot.layer = layer;
      slot.mask = mask;
      slot.isStatic = isStatic;
      return handle;
    } catch (const std::bad_alloc&) {
      if (!reuse && bodies.size() == static_cast<std::size_t>(index) + 1) bodies.pop_back();
      return {};
    }
  }

  void Remove(PhysicsBodyHandle handle) {
    BodySlot& slot = bodies[handle.index];

    bodyByID.erase(slot.id.GetIndexAndSequenceNumber());
    slot.id = {};
    slot.occupied = false;

    ++slot.generation;

    if (slot.generation == 0) {
      ++slot.generation;
    }

    freeSlots.push_back(handle.index);
  }
};

// ---------------------------------------------------------
// Construction
// ---------------------------------------------------------

JoltBackend::JoltBackend() : impl_(std::make_unique<Impl>()) {}

JoltBackend::~JoltBackend() { Shutdown(); }

PhysicsBackendType JoltBackend::GetType() const { return PhysicsBackendType::Jolt; }

// ---------------------------------------------------------
// Initialization
// ---------------------------------------------------------

bool JoltBackend::Initialize(const PhysicsWorldDesc& desc) {
  if (impl_->initialized) {
    return false;
  }

  if (desc.backend != PhysicsBackendType::Jolt || desc.maxBodies == 0 || desc.maxBodyPairs == 0 ||
      desc.maxContactConstraints == 0 || !IsFinite(desc.gravity)) {
    return false;
  }

  AcquireJolt();

  // Temporary memory used during physics simulation.
  // The initial 10 MiB allocation is insufficient for
  // the configured maximum contact capacity.
  constexpr std::uint32_t kTempAllocatorSize = 64u * 1024u * 1024u;

  impl_->allocator = std::make_unique<JPH::TempAllocatorImpl>(kTempAllocatorSize);

  const unsigned int hardwareThreads = std::thread::hardware_concurrency();

  const int workerThreads = static_cast<int>(hardwareThreads > 1 ? hardwareThreads - 1 : 1);

  impl_->jobs = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs,
                                                           JPH::cMaxPhysicsBarriers, workerThreads);

  impl_->physics = std::make_unique<JPH::PhysicsSystem>();

  impl_->collisionLayers.Register({
      .layer = 1,
      .mask = 0xFFFF,
      .isStatic = true,
  });

  impl_->collisionLayers.Register({
      .layer = 1,
      .mask = 0xFFFF,
      .isStatic = false,
  });

  impl_->physics->Init(desc.maxBodies, 0, desc.maxBodyPairs, desc.maxContactConstraints,
                       impl_->broadPhaseLayers, impl_->broadPhaseFilter, impl_->objectLayerFilter);

  impl_->physics->SetContactListener(&impl_->contactListener);

  impl_->physics->SetGravity(ToJolt(desc.gravity));
  impl_->gravity = desc.gravity;

  impl_->initialized = true;

  return true;
}

// ---------------------------------------------------------
// Shutdown
// ---------------------------------------------------------

void JoltBackend::Shutdown() {
  if (!impl_ || !impl_->initialized) {
    return;
  }

  impl_->physics->SetContactListener(nullptr);
  impl_->contactListener.Clear();
  impl_->contactPairs.Clear();
  impl_->contactEvents.Clear();

  for (auto& slot : impl_->constraints)
    if (slot.constraint) impl_->physics->RemoveConstraint(slot.constraint.GetPtr());
  impl_->constraints.clear();
  impl_->freeConstraints.clear();

  JPH::BodyInterface& bodyInterface = impl_->physics->GetBodyInterface();

  for (const Impl::BodySlot& slot : impl_->bodies) {
    if (!slot.occupied) {
      continue;
    }

    bodyInterface.RemoveBody(slot.id);
    bodyInterface.DestroyBody(slot.id);
  }

  impl_->bodies.clear();
  impl_->freeSlots.clear();
  impl_->bodyByID.clear();
  impl_->characters.clear();
  impl_->freeCharacters.clear();

  impl_->physics.reset();
  impl_->collisionLayers.Clear();
  impl_->jobs.reset();
  impl_->allocator.reset();

  impl_->initialized = false;

  ReleaseJolt();
}

// ---------------------------------------------------------
// Body creation
// ---------------------------------------------------------

PhysicsBodyHandle JoltBackend::CreateBody(const PhysicsBodyDesc& desc) {
  if (!impl_->initialized || !IsFinite(desc.transform.translation) ||
      !IsValidRotation(desc.transform.rotation) || !std::isfinite(desc.mass) ||
      !std::isfinite(desc.friction) || !std::isfinite(desc.restitution) || desc.friction < 0.0f ||
      desc.restitution < 0.0f || desc.restitution > 1.0f) {
    return {};
  }

  if (desc.motion == PhysicsMotionType::Dynamic && desc.mass <= 0.0f) {
    return {};
  }

  // Shape dimensions are specified separately.
  // Non-unit body scale is not supported.
  const Vector3 scale = desc.transform.scale;

  if (std::abs(scale.x - 1.0f) > 0.0001f || std::abs(scale.y - 1.0f) > 0.0001f ||
      std::abs(scale.z - 1.0f) > 0.0001f) {
    return {};
  }

  JPH::RefConst<JPH::Shape> shape =
      CreateShape(desc.shape, desc.motion == PhysicsMotionType::Static);

  if (!shape) {
    return {};
  }

  const JPH::EMotionType motion = ToMotionType(desc.motion);

  if (desc.layer == 0) {
    return {};
  }

  const CollisionFilter filter{
      .layer = desc.layer,
      .mask = desc.mask,
      .isStatic = motion == JPH::EMotionType::Static,
  };

  JPH::ObjectLayer layer{};

  try {
    layer = impl_->collisionLayers.Register(filter);
  } catch (const std::exception& error) {
    Logger::Warning(error.what());
    return {};
  }

  JPH::BodyCreationSettings settings(shape, ToJoltPosition(desc.transform.translation),
                                     ToNormalizedJolt(desc.transform.rotation), motion, layer);

  settings.mFriction = desc.friction;
  settings.mRestitution = desc.restitution;
  settings.mIsSensor = desc.isSensor;

  if (motion == JPH::EMotionType::Dynamic) {
    settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;

    settings.mMassPropertiesOverride.mMass = desc.mass;
  }

  JPH::BodyInterface& bodyInterface = impl_->physics->GetBodyInterface();

  const JPH::BodyID id = bodyInterface.CreateAndAddBody(
      settings, motion == JPH::EMotionType::Static ? JPH::EActivation::DontActivate
                                                   : JPH::EActivation::Activate);

  if (id.IsInvalid()) {
    return {};
  }

  const PhysicsBodyHandle handle =
      impl_->Store(id, desc.layer, desc.mask, motion == JPH::EMotionType::Static);

  if (!handle.IsValid()) {
    bodyInterface.RemoveBody(id);
    bodyInterface.DestroyBody(id);
    return {};
  }

  return handle;
}

// ---------------------------------------------------------
// Body destruction
// ---------------------------------------------------------

bool JoltBackend::DestroyBody(PhysicsBodyHandle handle) {
  if (!impl_->initialized) {
    return false;
  }

  const JPH::BodyID id = impl_->Resolve(handle);

  if (id.IsInvalid()) {
    return false;
  }

  for (std::uint32_t i = 0; i < impl_->constraints.size(); ++i) {
    const auto& slot = impl_->constraints[i];
    if (slot.constraint && (slot.bodyA == id || slot.bodyB == id))
      (void)DestroyConstraint({i, slot.generation});
  }

  // Translate terminal pairs before Jolt can invalidate the BodyID or its handle.
  impl_->Emit(impl_->contactPairs.EndForBody(id.GetIndexAndSequenceNumber()));
  const auto overflow = impl_->contactEvents.TakeOverflowCount();
  if (overflow != 0)
    Logger::Warning("Physics contact queue overflow: " + std::to_string(overflow) +
                    " events coalesced; terminal contacts retained.");
  JPH::BodyInterface& bodyInterface = impl_->physics->GetBodyInterface();
  bodyInterface.RemoveBody(id);
  bodyInterface.DestroyBody(id);
  impl_->Remove(handle);

  return true;
}

// ---------------------------------------------------------
// Transforms
// ---------------------------------------------------------

bool JoltBackend::GetTransform(PhysicsBodyHandle handle, Transform& outTransform) const {
  if (!impl_->initialized) {
    return false;
  }

  const JPH::BodyID id = impl_->Resolve(handle);

  if (id.IsInvalid()) {
    return false;
  }

  const JPH::BodyInterface& bodyInterface = impl_->physics->GetBodyInterface();

  outTransform.translation = FromJoltPosition(bodyInterface.GetPosition(id));

  outTransform.rotation = FromJolt(bodyInterface.GetRotation(id));

  outTransform.scale = {1.0f, 1.0f, 1.0f};

  return true;
}

bool JoltBackend::SetTransform(PhysicsBodyHandle handle, const Transform& transform) {
  if (!impl_->initialized || !IsFinite(transform.translation) ||
      !IsValidRotation(transform.rotation)) {
    return false;
  }

  if (std::abs(transform.scale.x - 1.0f) > 0.0001f ||
      std::abs(transform.scale.y - 1.0f) > 0.0001f ||
      std::abs(transform.scale.z - 1.0f) > 0.0001f) {
    return false;
  }

  const JPH::BodyID id = impl_->Resolve(handle);

  if (id.IsInvalid()) {
    return false;
  }

  impl_->physics->GetBodyInterface().SetPositionAndRotation(
      id, ToJoltPosition(transform.translation), ToNormalizedJolt(transform.rotation),
      JPH::EActivation::Activate);

  return true;
}

// ---------------------------------------------------------
// Velocity and forces
// ---------------------------------------------------------

bool JoltBackend::SetLinearVelocity(PhysicsBodyHandle handle, Vector3 velocity) {
  if (!impl_->initialized || !IsFinite(velocity)) {
    return false;
  }

  const JPH::BodyID id = impl_->Resolve(handle);

  if (id.IsInvalid()) {
    return false;
  }

  impl_->physics->GetBodyInterface().SetLinearVelocity(id, ToJolt(velocity));

  return true;
}

bool JoltBackend::GetLinearVelocity(PhysicsBodyHandle handle, Vector3& outVelocity) const {
  if (!impl_->initialized) {
    return false;
  }

  const JPH::BodyID id = impl_->Resolve(handle);

  if (id.IsInvalid()) {
    return false;
  }

  outVelocity = FromJolt(impl_->physics->GetBodyInterface().GetLinearVelocity(id));

  return true;
}

bool JoltBackend::AddForce(PhysicsBodyHandle handle, Vector3 force) {
  if (!impl_->initialized || !IsFinite(force)) {
    return false;
  }

  const JPH::BodyID id = impl_->Resolve(handle);

  if (id.IsInvalid()) {
    return false;
  }

  impl_->physics->GetBodyInterface().AddForce(id, ToJolt(force));

  return true;
}

bool JoltBackend::AddImpulse(PhysicsBodyHandle handle, Vector3 impulse) {
  if (!impl_->initialized || !IsFinite(impulse)) {
    return false;
  }

  const JPH::BodyID id = impl_->Resolve(handle);

  if (id.IsInvalid()) {
    return false;
  }

  impl_->physics->GetBodyInterface().AddImpulse(id, ToJolt(impulse));

  return true;
}

// ---------------------------------------------------------
// Raycasting
// ---------------------------------------------------------

bool JoltBackend::Raycast(Vector3 origin, Vector3 direction, float maxDistance,
                          PhysicsRaycastHit& outHit) const {
  outHit = {};

  if (!impl_->initialized || !IsFinite(origin) || !IsFinite(direction) ||
      !std::isfinite(maxDistance) || maxDistance <= 0.0f) {
    return false;
  }

  const float length =
      std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);

  if (length <= 0.000001f) {
    return false;
  }

  const float scale = maxDistance / length;

  const Vector3 displacement{direction.x * scale, direction.y * scale, direction.z * scale};

  JPH::RRayCast ray(ToJoltPosition(origin), ToJolt(displacement));

  JPH::RayCastResult result;

  if (!impl_->physics->GetNarrowPhaseQuery().CastRay(ray, result)) {
    return false;
  }

  const PhysicsBodyHandle handle = impl_->Find(result.mBodyID);

  if (!handle.IsValid()) {
    return false;
  }

  outHit.body = handle;
  outHit.fraction = result.mFraction;

  outHit.position = FromJoltPosition(ray.GetPointOnRay(result.mFraction));

  const JPH::BodyLockRead lock(impl_->physics->GetBodyLockInterface(), result.mBodyID);

  if (lock.Succeeded()) {
    outHit.normal = FromJolt(lock.GetBody().GetWorldSpaceSurfaceNormal(
        result.mSubShapeID2, ray.GetPointOnRay(result.mFraction)));
  }

  return true;
}

bool JoltBackend::RaycastFiltered(Vector3 origin, Vector3 direction, float maxDistance,
                                  const PhysicsRaycastFilter& filter,
                                  PhysicsRaycastHit& outHit) const {
  outHit = {};

  if (!impl_->initialized || !IsFinite(origin) || !IsFinite(direction) ||
      !std::isfinite(maxDistance) || maxDistance <= 0.0f || filter.mask == 0) {
    return false;
  }

  const float length =
      std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);

  if (length <= 0.000001f) {
    return false;
  }

  const float scale = maxDistance / length;

  const Vector3 displacement{direction.x * scale, direction.y * scale, direction.z * scale};

  const JPH::RRayCast ray(ToJoltPosition(origin), ToJolt(displacement));

  const RaycastObjectLayerFilter layerFilter(impl_->collisionLayers, filter.mask);

  const RaycastSensorFilter sensorFilter(filter.includeSensors);

  JPH::RayCastResult result{};

  if (!impl_->physics->GetNarrowPhaseQuery().CastRay(ray, result, JPH::BroadPhaseLayerFilter{},
                                                     layerFilter, sensorFilter)) {
    return false;
  }

  const PhysicsBodyHandle handle = impl_->Find(result.mBodyID);

  if (!handle.IsValid()) {
    return false;
  }

  outHit.body = handle;
  outHit.fraction = result.mFraction;
  outHit.position = FromJoltPosition(ray.GetPointOnRay(result.mFraction));

  const JPH::BodyLockRead lock(impl_->physics->GetBodyLockInterface(), result.mBodyID);

  if (lock.Succeeded()) {
    outHit.normal = FromJolt(lock.GetBody().GetWorldSpaceSurfaceNormal(
        result.mSubShapeID2, ray.GetPointOnRay(result.mFraction)));
  }

  return true;
}

bool JoltBackend::ShapeCast(const PhysicsShapeDesc& shapeDesc, const Transform& start,
                            Vector3 direction, float maxDistance, const PhysicsQueryFilter& filter,
                            PhysicsShapeCastHit& output) const {
  output = {};
  if (shapeDesc.type == PhysicsShapeType::TriangleMesh ||
      shapeDesc.type == PhysicsShapeType::Compound)
    return false;
  if (!impl_->initialized || !ValidQueryTransform(start) || !IsFinite(direction) ||
      !std::isfinite(maxDistance) || maxDistance <= 0.0f || filter.mask == 0)
    return false;
  const float length = Vector3Length(direction);
  if (!std::isfinite(length) || length <= 0.000001f) return false;
  const auto ignored =
      filter.ignoredBody.IsValid() ? impl_->Resolve(filter.ignoredBody) : JPH::BodyID{};
  if (filter.ignoredBody.IsValid() && ignored.IsInvalid()) return false;
  const auto shape = CreateShape(shapeDesc);
  if (!shape) return false;
  const Vector3 travel = Vector3Scale(direction, maxDistance / length);
  const auto transform = JPH::RMat44::sRotationTranslation(ToNormalizedJolt(start.rotation),
                                                           ToJoltPosition(start.translation));
  const auto cast = JPH::RShapeCast::sFromWorldTransform(
      shape.GetPtr(), JPH::Vec3::sReplicate(1.0f), transform, ToJolt(travel));
  JPH::ShapeCastSettings settings;
  settings.mReturnDeepestPoint = true;
  JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
  const RaycastObjectLayerFilter layerFilter(impl_->collisionLayers, filter.mask);
  const QueryBodyFilter bodyFilter(filter.includeSensors, ignored);
  impl_->physics->GetNarrowPhaseQuery().CastShape(cast, settings, JPH::RVec3::sZero(), collector,
                                                  JPH::BroadPhaseLayerFilter{}, layerFilter,
                                                  bodyFilter);
  const JPH::ShapeCastResult* best = nullptr;
  PhysicsBodyHandle bestHandle{};
  for (const auto& hit : collector.mHits) {
    const auto handle = impl_->Find(hit.mBodyID2);
    if (!handle.IsValid()) continue;
    if (!best || hit.mFraction < best->mFraction ||
        (hit.mFraction == best->mFraction && handle.index < bestHandle.index)) {
      best = &hit;
      bestHandle = handle;
    }
  }
  if (!best) return false;
  output.body = bestHandle;
  output.fraction = best->mFraction;
  output.position = FromJolt(best->mContactPointOn2);
  const auto axis = best->mPenetrationAxis;
  output.normal = axis.LengthSq() > 0.000001f ? FromJolt(-axis.Normalized())
                                              : Vector3Scale(direction, -1.0f / length);
  return true;
}

bool JoltBackend::OverlapShape(const PhysicsShapeDesc& shapeDesc, const Transform& transform,
                               const PhysicsQueryFilter& filter, PhysicsBodyHandle* results,
                               std::uint32_t capacity, std::uint32_t& count) const {
  count = 0;
  if (!impl_->initialized || !ValidQueryTransform(transform) || filter.mask == 0 ||
      (capacity != 0 && !results))
    return false;
  const auto ignored =
      filter.ignoredBody.IsValid() ? impl_->Resolve(filter.ignoredBody) : JPH::BodyID{};
  if (filter.ignoredBody.IsValid() && ignored.IsInvalid()) return false;
  const auto shape = CreateShape(shapeDesc);
  if (!shape) return false;
  const auto worldTransform = JPH::RMat44::sRotationTranslation(
      ToNormalizedJolt(transform.rotation), ToJoltPosition(transform.translation));
  const auto centerTransform = worldTransform.PreTranslated(shape->GetCenterOfMass());
  JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
  const RaycastObjectLayerFilter layerFilter(impl_->collisionLayers, filter.mask);
  const QueryBodyFilter bodyFilter(filter.includeSensors, ignored);
  impl_->physics->GetNarrowPhaseQuery().CollideShape(
      shape.GetPtr(), JPH::Vec3::sReplicate(1.0f), centerTransform, JPH::CollideShapeSettings{},
      JPH::RVec3::sZero(), collector, JPH::BroadPhaseLayerFilter{}, layerFilter, bodyFilter);
  std::vector<PhysicsBodyHandle> unique;
  unique.reserve(collector.mHits.size());
  for (const auto& hit : collector.mHits) {
    const auto handle = impl_->Find(hit.mBodyID2);
    if (handle.IsValid()) unique.push_back(handle);
  }
  std::sort(unique.begin(), unique.end(), [](auto a, auto b) {
    return a.index < b.index || (a.index == b.index && a.generation < b.generation);
  });
  unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
  count = static_cast<std::uint32_t>(unique.size());
  for (std::uint32_t i = 0; i < std::min(count, capacity); ++i) results[i] = unique[i];
  return true;
}

PhysicsCharacterHandle JoltBackend::CreateCharacter(const PhysicsCharacterDesc& desc) {
  if (!impl_->initialized || !IsFinite(desc.position) || !std::isfinite(desc.radius) ||
      !std::isfinite(desc.height) || !std::isfinite(desc.maxSlopeAngle) ||
      !std::isfinite(desc.stepHeight) || !std::isfinite(desc.groundSnapDistance) ||
      desc.radius <= 0 || desc.height <= 2 * desc.radius || desc.maxSlopeAngle < 0 ||
      desc.maxSlopeAngle >= 90 || desc.stepHeight < 0 || desc.groundSnapDistance < 0 ||
      desc.layer == 0)
    return {};
  JPH::CharacterVirtualSettings settings;
  settings.mShape = new JPH::CapsuleShape(0.5f * desc.height - desc.radius, desc.radius);
  settings.mMaxSlopeAngle = JPH::DegreesToRadians(desc.maxSlopeAngle);
  const bool reuse = !impl_->freeCharacters.empty();
  if (!reuse && impl_->characters.size() >= std::numeric_limits<std::uint32_t>::max()) return {};
  const auto index =
      reuse ? impl_->freeCharacters.back() : static_cast<std::uint32_t>(impl_->characters.size());
  try {
    JPH::Ref<JPH::CharacterVirtual> character = new JPH::CharacterVirtual(
        &settings, ToJoltPosition(desc.position), JPH::Quat::sIdentity(), impl_->physics.get());

    if (!reuse) impl_->characters.emplace_back();

    if (reuse) impl_->freeCharacters.pop_back();

    character->SetCharacterVsCharacterCollision(&impl_->characterCollisions);

    impl_->characterCollisions.Add(character.GetPtr());

    auto& slot = impl_->characters[index];
    slot.character = character;
    slot.desc = desc;
    slot.desiredHorizontal = {};
    slot.actualVelocity = {};
    slot.airborneCarry = {};
    slot.verticalSpeed = 0;
    slot.jumping = false;
    CharacterObjectFilter objectFilter(impl_->collisionLayers, desc.layer, desc.mask);
    QueryBodyFilter bodyFilter(false, {});
    slot.character->RefreshContacts(JPH::BroadPhaseLayerFilter{}, objectFilter, bodyFilter,
                                    JPH::ShapeFilter{}, *impl_->allocator);
    return {index, slot.generation};
  } catch (const std::bad_alloc&) {
    return {};
  }
}

bool JoltBackend::DestroyCharacter(PhysicsCharacterHandle handle) {
  auto* slot = impl_->ResolveCharacter(handle);
  if (!slot) return false;
  impl_->characterCollisions.Remove(slot->character.GetPtr());
  slot->character = nullptr;
  slot->generation =
      slot->generation == std::numeric_limits<std::uint32_t>::max() ? 1 : slot->generation + 1;
  try {
    impl_->freeCharacters.push_back(handle.index);
  } catch (const std::bad_alloc&) {
    // The vacant slot remains generation-invalid and can be reclaimed at shutdown.
  }
  return true;
}

bool JoltBackend::SetCharacterVelocity(PhysicsCharacterHandle handle, Vector3 horizontal) {
  auto* slot = impl_->ResolveCharacter(handle);
  if (!slot || !IsFinite(horizontal) || horizontal.y != 0) return false;
  slot->desiredHorizontal = horizontal;
  return true;
}

bool JoltBackend::JumpCharacter(PhysicsCharacterHandle handle, float speed) {
  auto* slot = impl_->ResolveCharacter(handle);
  if (!slot || !std::isfinite(speed) || speed <= 0 ||
      slot->character->GetGroundState() != JPH::CharacterBase::EGroundState::OnGround)
    return false;
  slot->verticalSpeed = speed;
  slot->jumping = true;
  return true;
}

bool JoltBackend::SetCharacterPosition(PhysicsCharacterHandle handle, Vector3 position) {
  auto* slot = impl_->ResolveCharacter(handle);
  if (!slot || !IsFinite(position)) return false;
  slot->character->SetPosition(ToJoltPosition(position));
  slot->verticalSpeed = 0;
  slot->actualVelocity = {};
  slot->airborneCarry = {};
  slot->jumping = false;
  CharacterObjectFilter objectFilter(impl_->collisionLayers, slot->desc.layer, slot->desc.mask);
  QueryBodyFilter bodyFilter(false, {});
  slot->character->RefreshContacts(JPH::BroadPhaseLayerFilter{}, objectFilter, bodyFilter,
                                   JPH::ShapeFilter{}, *impl_->allocator);
  return true;
}

bool JoltBackend::GetCharacterPosition(PhysicsCharacterHandle handle, Vector3& output) const {
  const auto* slot = impl_->ResolveCharacter(handle);
  if (!slot) return false;
  output = FromJoltPosition(slot->character->GetPosition());
  return true;
}

bool JoltBackend::GetCharacterVelocity(PhysicsCharacterHandle handle, Vector3& output) const {
  const auto* slot = impl_->ResolveCharacter(handle);
  if (!slot) return false;
  output = slot->actualVelocity;
  return true;
}

bool JoltBackend::GetCharacterGroundState(PhysicsCharacterHandle handle,
                                          PhysicsGroundState& output) const {
  const auto* slot = impl_->ResolveCharacter(handle);
  if (!slot) return false;
  switch (slot->character->GetGroundState()) {
    case JPH::CharacterBase::EGroundState::OnGround:
      output = PhysicsGroundState::Grounded;
      break;
    case JPH::CharacterBase::EGroundState::OnSteepGround:
      output = PhysicsGroundState::Sliding;
      break;
    default:
      output = PhysicsGroundState::Airborne;
  }
  return true;
}

bool JoltBackend::GetCharacterGroundNormal(PhysicsCharacterHandle handle, Vector3& output) const {
  const auto* slot = impl_->ResolveCharacter(handle);
  if (!slot) return false;
  output = FromJolt(slot->character->GetGroundNormal());
  return true;
}

PhysicsConstraintHandle JoltBackend::CreateConstraint(const PhysicsConstraintDesc& desc) {
  if (!impl_->initialized || !desc.bodyA.IsValid() || !desc.bodyB.IsValid() ||
      desc.bodyA == desc.bodyB || !IsFinite(desc.anchor) || !IsFinite(desc.axis) ||
      !IsFinite(desc.normal) || !ValidMotor(desc.motor))
    return {};
  const auto idA = impl_->Resolve(desc.bodyA);
  const auto idB = impl_->Resolve(desc.bodyB);
  if (idA.IsInvalid() || idB.IsInvalid()) return {};
  const bool axisNeeded =
      desc.type == PhysicsConstraintType::Hinge || desc.type == PhysicsConstraintType::Slider;
  if (axisNeeded && Vector3LengthSqr(desc.axis) < 0.000001f) return {};
  if (desc.type == PhysicsConstraintType::Hinge &&
      (Vector3LengthSqr(desc.normal) < 0.000001f ||
       std::abs(Vector3DotProduct(Vector3Normalize(desc.axis), Vector3Normalize(desc.normal))) >
           0.01f))
    return {};
  if (desc.enableLimits &&
      (!axisNeeded || !std::isfinite(desc.limitMin) || !std::isfinite(desc.limitMax) ||
       desc.limitMin > 0 || desc.limitMax < 0 || desc.limitMin >= desc.limitMax ||
       (desc.type == PhysicsConstraintType::Hinge && (desc.limitMin < -PI || desc.limitMax > PI))))
    return {};
  if (!axisNeeded && desc.motor.mode != PhysicsMotorMode::Off) return {};
  if (desc.type != PhysicsConstraintType::Fixed && desc.type != PhysicsConstraintType::Point &&
      desc.type != PhysicsConstraintType::Hinge && desc.type != PhysicsConstraintType::Slider)
    return {};

  JPH::Ref<JPH::TwoBodyConstraint> constraint;
  const JPH::BodyID ids[]{idA, idB};
  {
    JPH::BodyLockMultiWrite locks(impl_->physics->GetBodyLockInterface(), ids, 2);
    JPH::Body* bodyA = locks.GetBody(0);
    JPH::Body* bodyB = locks.GetBody(1);
    if (!bodyA || !bodyB ||
        (bodyA->GetMotionType() == JPH::EMotionType::Static &&
         bodyB->GetMotionType() == JPH::EMotionType::Static))
      return {};
    const auto anchor = ToJoltPosition(desc.anchor);
    if (desc.type == PhysicsConstraintType::Fixed) {
      JPH::FixedConstraintSettings settings;
      settings.mAutoDetectPoint = true;
      constraint = settings.Create(*bodyA, *bodyB);
    } else if (desc.type == PhysicsConstraintType::Point) {
      JPH::PointConstraintSettings settings;
      settings.mPoint1 = anchor;
      settings.mPoint2 = anchor;
      constraint = settings.Create(*bodyA, *bodyB);
    } else if (desc.type == PhysicsConstraintType::Hinge) {
      JPH::HingeConstraintSettings settings;
      settings.mPoint1 = anchor;
      settings.mPoint2 = anchor;
      settings.mHingeAxis1 = ToJolt(Vector3Normalize(desc.axis));
      settings.mHingeAxis2 = settings.mHingeAxis1;
      settings.mNormalAxis1 = ToJolt(Vector3Normalize(desc.normal));
      settings.mNormalAxis2 = settings.mNormalAxis1;
      if (desc.enableLimits) {
        settings.mLimitsMin = desc.limitMin;
        settings.mLimitsMax = desc.limitMax;
      }
      constraint = settings.Create(*bodyA, *bodyB);
    } else {
      JPH::SliderConstraintSettings settings;
      settings.mPoint1 = anchor;
      settings.mPoint2 = anchor;
      settings.SetSliderAxis(ToJolt(Vector3Normalize(desc.axis)));
      if (desc.enableLimits) {
        settings.mLimitsMin = desc.limitMin;
        settings.mLimitsMax = desc.limitMax;
      }
      constraint = settings.Create(*bodyA, *bodyB);
    }
  }
  if (!constraint || (axisNeeded && !ConfigureMotor(constraint.GetPtr(), desc.type, desc.motor)))
    return {};
  const bool reuse = !impl_->freeConstraints.empty();
  if (!reuse && impl_->constraints.size() >= std::numeric_limits<std::uint32_t>::max()) return {};
  const auto index =
      reuse ? impl_->freeConstraints.back() : static_cast<std::uint32_t>(impl_->constraints.size());
  try {
    if (!reuse) impl_->constraints.emplace_back();
  } catch (const std::bad_alloc&) {
    return {};
  }
  if (reuse) impl_->freeConstraints.pop_back();
  auto& slot = impl_->constraints[index];
  slot.constraint = constraint;
  slot.bodyA = idA;
  slot.bodyB = idB;
  slot.type = desc.type;
  slot.suppressCollision = !desc.collideConnected;
  if (slot.suppressCollision) impl_->contactListener.Suppress(idA, idB);
  impl_->physics->AddConstraint(constraint.GetPtr());
  impl_->physics->GetBodyInterface().InvalidateContactCache(idA);
  return {index, slot.generation};
}

bool JoltBackend::DestroyConstraint(PhysicsConstraintHandle handle) {
  auto* slot = impl_->ResolveConstraint(handle);
  if (!impl_->initialized || !slot) return false;
  impl_->physics->RemoveConstraint(slot->constraint.GetPtr());
  if (slot->suppressCollision) impl_->contactListener.Unsuppress(slot->bodyA, slot->bodyB);
  impl_->physics->GetBodyInterface().InvalidateContactCache(slot->bodyA);
  slot->constraint = nullptr;
  slot->generation =
      slot->generation == std::numeric_limits<std::uint32_t>::max() ? 1 : slot->generation + 1;
  try {
    impl_->freeConstraints.push_back(handle.index);
  } catch (const std::bad_alloc&) {
  }
  return true;
}

bool JoltBackend::SetConstraintMotor(PhysicsConstraintHandle handle,
                                     const PhysicsMotorDesc& motor) {
  auto* slot = impl_->ResolveConstraint(handle);
  if (!impl_->initialized || !slot || !ConfigureMotor(slot->constraint.GetPtr(), slot->type, motor))
    return false;
  auto& bodies = impl_->physics->GetBodyInterface();
  if (bodies.GetMotionType(slot->bodyA) == JPH::EMotionType::Dynamic)
    bodies.ActivateBody(slot->bodyA);
  if (bodies.GetMotionType(slot->bodyB) == JPH::EMotionType::Dynamic)
    bodies.ActivateBody(slot->bodyB);
  return true;
}

bool JoltBackend::SetCollisionFilter(PhysicsBodyHandle handle, std::uint16_t layer,
                                     std::uint16_t mask) {
  if (!impl_->initialized || layer == 0) {
    return false;
  }

  const JPH::BodyID id = impl_->Resolve(handle);
  if (id.IsInvalid()) {
    return false;
  }

  auto& slot = impl_->bodies[handle.index];

  if (slot.layer == layer && slot.mask == mask) {
    return true;
  }

  const auto previousLayer = slot.layer;
  const auto previousMask = slot.mask;

  const CollisionFilter filter{
      .layer = layer,
      .mask = mask,
      .isStatic = slot.isStatic,
  };

  JPH::ObjectLayer objectLayer{};

  try {
    objectLayer = impl_->collisionLayers.Register(filter);
  } catch (const std::exception& error) {
    Logger::Warning(error.what());
    return false;
  }

  auto& bodyInterface = impl_->physics->GetBodyInterface();
  // Complete observations from the previous step before changing pair eligibility.
  impl_->contactListener.Take(impl_->observationScratch);
  impl_->ApplyObservations();

  bodyInterface.SetObjectLayer(id, objectLayer);
  if (bodyInterface.GetObjectLayer(id) != objectLayer) return false;

  // Registry state is host-owned; Jolt may remove its cached subshape contacts later.
  // Those delayed removals cannot emit another End for the terminated pairs.
  impl_->Emit(impl_->contactPairs.EndFilteredPairs(
      id.GetIndexAndSequenceNumber(), [&](std::uint32_t otherID) {
        const auto it = impl_->bodyByID.find(otherID);
        if (it == impl_->bodyByID.end()) return true;
        const auto& other = impl_->bodies[it->second.index];
        return (slot.isStatic && other.isStatic) || (mask & other.layer) == 0 ||
               (other.mask & layer) == 0;
      }));

  slot.layer = layer;
  slot.mask = mask;

  // Layer changes notify the broadphase. Wake newly compatible moving bodies
  // so overlapping pairs are tested even when both were sleeping.
  bodyInterface.InvalidateContactCache(id);
  bool newlyPermitted = false;
  for (const auto& other : impl_->bodies) {
    if (!other.occupied || other.id == id || (slot.isStatic && other.isStatic)) continue;
    const bool permitted = (mask & other.layer) != 0 && (other.mask & layer) != 0;
    const bool previouslyPermitted =
        (previousMask & other.layer) != 0 && (other.mask & previousLayer) != 0;
    if (permitted && !previouslyPermitted) {
      newlyPermitted = true;
      if (!other.isStatic) bodyInterface.ActivateBody(other.id);
    }
  }
  if (newlyPermitted && !slot.isStatic) bodyInterface.ActivateBody(id);

  return true;
}

// ---------------------------------------------------------
// Fixed-step simulation
// ---------------------------------------------------------

void JoltBackend::Step(float fixedDeltaTime) {
  if (!impl_->initialized || !std::isfinite(fixedDeltaTime) || fixedDeltaTime <= 0.0f) {
    return;
  }

  impl_->physics->Update(fixedDeltaTime, 1, impl_->allocator.get(), impl_->jobs.get());

  for (auto& slot : impl_->characters) {
    if (!slot.character) continue;
    auto& character = *slot.character;
    const auto previous = character.GetPosition();
    const bool grounded = character.GetGroundState() == JPH::CharacterBase::EGroundState::OnGround;
    if (grounded && !slot.jumping && slot.verticalSpeed <= 0)
      slot.verticalSpeed = 0;
    else
      slot.verticalSpeed += impl_->gravity.y * fixedDeltaTime;
    if (grounded) character.UpdateGroundVelocity();
    const auto groundVelocity = grounded ? character.GetGroundVelocity() : JPH::Vec3::sZero();
    if (grounded)
      slot.airborneCarry =
          slot.jumping ? Vector3{groundVelocity.GetX(), 0, groundVelocity.GetZ()} : Vector3{};
    const Vector3 carry = grounded ? FromJolt(groundVelocity) : slot.airborneCarry;

    character.SetLinearVelocity({slot.desiredHorizontal.x + carry.x,
                                 slot.verticalSpeed + groundVelocity.GetY(),
                                 slot.desiredHorizontal.z + carry.z});
    CharacterObjectFilter objectFilter(impl_->collisionLayers, slot.desc.layer, slot.desc.mask);
    QueryBodyFilter bodyFilter(false, {});
    JPH::CharacterVirtual::ExtendedUpdateSettings settings;
    settings.mWalkStairsStepUp = {0, slot.desc.stepHeight, 0};
    settings.mStickToFloorStepDown = slot.jumping && slot.verticalSpeed > 0
                                         ? JPH::Vec3::sZero()
                                         : JPH::Vec3(0, -slot.desc.groundSnapDistance, 0);
    character.ExtendedUpdate(fixedDeltaTime, ToJolt(impl_->gravity), settings,
                             JPH::BroadPhaseLayerFilter{}, objectFilter, bodyFilter,
                             JPH::ShapeFilter{}, *impl_->allocator);
    const auto displacement = character.GetPosition() - previous;
    slot.actualVelocity = FromJolt(displacement / fixedDeltaTime);
    if (slot.verticalSpeed > 0 && slot.actualVelocity.y < 0.01f) slot.verticalSpeed = 0;
    if (slot.verticalSpeed <= 0) slot.jumping = false;
  }

  impl_->contactListener.Take(impl_->observationScratch);
  impl_->ApplyObservations();
  const auto overflow = impl_->contactEvents.TakeOverflowCount();
  if (overflow != 0)
    Logger::Warning("Physics contact queue overflow: " + std::to_string(overflow) +
                    " events coalesced; terminal contacts retained.");
}

bool JoltBackend::PollContactEvent(PhysicsContactEvent& output) {
  return impl_->initialized && impl_->contactEvents.Poll(output);
}

std::uint32_t JoltBackend::ActiveContactPairCount() const {
  return impl_->initialized ? static_cast<std::uint32_t>(impl_->contactPairs.ActivePairCount()) : 0;
}

}  // namespace Levye
