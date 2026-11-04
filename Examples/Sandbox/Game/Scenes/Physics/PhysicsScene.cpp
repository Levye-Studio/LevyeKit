
#include "PhysicsScene.hpp"

#ifdef LEVYE_WITH_PHYSICS

#include <Levye/LevyeKit.hpp>

namespace Sandbox {

bool PhysicsScene::Initialize() {
  if (ready) return true;

  if (!Levye::Physics::IsAvailable()) {
    Levye::Log::Error("Physics: host module unavailable");
    return false;
  }

  Levye::PhysicsWorldDesc worldDesc{};
  worldDesc.backend = Levye::PhysicsBackendType::Jolt;
  worldDesc.gravity = {0.0f, -9.81f, 0.0f};

  world = Levye::Physics::CreateWorld(worldDesc);
  if (!world.IsValid()) return false;

  Levye::PhysicsBodyDesc floorDesc{};
  floorDesc.motion = Levye::PhysicsMotionType::Static;
  floorDesc.shape.type = Levye::PhysicsShapeType::Box;
  floorDesc.shape.halfExtents = {10.0f, 0.5f, 10.0f};
  floorDesc.transform.translation = {0.0f, -0.5f, 0.0f};

  floor = Levye::Physics::CreateBody(world, floorDesc);

  Levye::PhysicsBodyDesc ballDesc{};
  ballDesc.motion = Levye::PhysicsMotionType::Dynamic;
  ballDesc.shape.type = Levye::PhysicsShapeType::Sphere;
  ballDesc.shape.radius = 0.5f;
  ballDesc.transform.translation = {0.0f, 5.0f, 0.0f};
  ballDesc.mass = 1.0f;
  ballDesc.restitution = 0.7f;

  ball = Levye::Physics::CreateBody(world, ballDesc);

  ready = floor.IsValid() && ball.IsValid();

  if (!ready) {
    Shutdown();
    return false;
  }

  Levye::Log::Info("Physics: scene initialized");
  return true;
}

void PhysicsScene::Update() {
  if (!ready) return;

  Transform transform{};

  if (Levye::Physics::GetTransform(world, ball, transform)) {
    ballPosition = transform.translation;
  }

  if (Levye::Input::IsPressed("PhysicsImpulse")) {
    (void)Levye::Physics::AddImpulse(world, ball, {0.0f, 6.0f, 0.0f});
  }

  if (Levye::Input::IsPressed("PhysicsReset")) {
    Transform reset{};
    reset.translation = {0.0f, 5.0f, 0.0f};
    reset.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    reset.scale = {1.0f, 1.0f, 1.0f};

    (void)Levye::Physics::SetTransform(world, ball, reset);
    (void)Levye::Physics::SetLinearVelocity(world, ball, {0.0f, 0.0f, 0.0f});
  }
}

void PhysicsScene::Draw() const {
  ClearBackground({18, 22, 32, 255});

  Camera3D camera{};
  camera.position = {12.0f, 9.0f, 12.0f};
  camera.target = {0.0f, 1.0f, 0.0f};
  camera.up = {0.0f, 1.0f, 0.0f};
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  BeginMode3D(camera);

  DrawGrid(20, 1.0f);

  DrawCube({0.0f, -0.5f, 0.0f}, 20.0f, 1.0f, 20.0f, DARKGRAY);

  if (ready) {
    DrawSphere(ballPosition, 0.5f, ORANGE);
  }

  EndMode3D();

  DrawText("LEVYEKIT PHYSICS", 20, 20, 30, RAYWHITE);
  DrawText("SPACE: Impulse | R: Reset | ESC: Menu", 20, 60, 20, LIGHTGRAY);

  DrawText(TextFormat("Ball Y: %.3f", ballPosition.y), 20, 95, 20, WHITE);

  DrawText(TextFormat("Hot reloads: %d", reloadCount), 20, 125, 20, GREEN);

  if (!ready) {
    DrawText("Physics unavailable", 20, 165, 20, RED);
  }
}

void PhysicsScene::BeforeReload() {
  reloadSnapshotValid = false;
  if (!ready) return;

  Transform transform{};

  const bool positionOK = Levye::Physics::GetTransform(world, ball, transform);

  const bool velocityOK = Levye::Physics::GetLinearVelocity(world, ball, velocityBeforeReload);

  if (positionOK) {
    positionBeforeReload = transform.translation;
  }

  reloadSnapshotValid = positionOK && velocityOK;
}

bool PhysicsScene::AfterReload() {
  ++reloadCount;

  if (!ready || !reloadSnapshotValid) return false;

  Transform transform{};
  Vector3 velocity{};

  const bool positionOK = Levye::Physics::GetTransform(world, ball, transform);

  const bool velocityOK = Levye::Physics::GetLinearVelocity(world, ball, velocity);

  if (!positionOK || !velocityOK) return false;

  // No physics step should run between the two reload callbacks.
  const float tolerance = 0.0001f;

  const bool samePosition =
      Vector3Distance(transform.translation, positionBeforeReload) < tolerance;

  const bool sameVelocity = Vector3Distance(velocity, velocityBeforeReload) < tolerance;

  return samePosition && sameVelocity;
}

void PhysicsScene::Shutdown() {
  if (world.IsValid()) {
    (void)Levye::Physics::DestroyWorld(world);
  }

  world = {};
  floor = {};
  ball = {};
  ready = false;
  reloadSnapshotValid = false;
}

}  // namespace Sandbox

#endif
