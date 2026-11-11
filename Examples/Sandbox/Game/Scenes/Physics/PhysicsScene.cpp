
#include "PhysicsScene.hpp"

#include <rlgl.h>

#include <Levye/LevyeKit.hpp>
#include <algorithm>
#include <cmath>
#ifdef LEVYE_WITH_PHYSICS

constexpr Vector3 platformTestSpawn{26.0f, 2.5f, -3.0f};

namespace PhysicsLayers {
constexpr std::uint16_t World = 1u << 0;
constexpr std::uint16_t Ball = 1u << 1;
constexpr std::uint16_t Trigger = 1u << 2;
constexpr std::uint16_t Character = 1u << 3;
}  // namespace PhysicsLayers

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
  floorDesc.layer = PhysicsLayers::World;
  floorDesc.mask = PhysicsLayers::Ball;

  floor = Levye::Physics::CreateBody(world, floorDesc);

  Levye::PhysicsBodyDesc ballDesc{};
  ballDesc.motion = Levye::PhysicsMotionType::Dynamic;
  ballDesc.shape.type = Levye::PhysicsShapeType::Sphere;
  ballDesc.shape.radius = 0.5f;
  ballDesc.transform.translation = {0.0f, 5.0f, 0.0f};
  ballDesc.mass = 1.0f;
  ballDesc.restitution = 0.7f;
  ballDesc.layer = PhysicsLayers::Ball;
  ballDesc.mask = PhysicsLayers::World | PhysicsLayers::Trigger;

  ball = Levye::Physics::CreateBody(world, ballDesc);

  Levye::PhysicsBodyDesc sensorDesc{};

  sensorDesc.motion = Levye::PhysicsMotionType::Static;
  sensorDesc.shape.type = Levye::PhysicsShapeType::Box;
  sensorDesc.shape.halfExtents = {2.0f, 0.75f, 2.0f};

  sensorDesc.transform.translation = {0.0f, 2.5f, 0.0f};
  sensorDesc.layer = PhysicsLayers::Trigger;
  sensorDesc.mask = PhysicsLayers::Ball;

  sensorDesc.isSensor = true;

  sensor = Levye::Physics::CreateBody(world, sensorDesc);

  auto makeObstacle = [&](Vector3 position, Vector3 halfExtents,
                          Quaternion rotation = {0, 0, 0, 1}) {
    Levye::PhysicsBodyDesc obstacle{};
    obstacle.motion = Levye::PhysicsMotionType::Static;
    obstacle.shape.type = Levye::PhysicsShapeType::Box;
    obstacle.shape.halfExtents = halfExtents;
    obstacle.transform.translation = position;
    obstacle.transform.rotation = rotation;
    obstacle.layer = PhysicsLayers::World;
    obstacle.mask = PhysicsLayers::Character;
    return Levye::Physics::CreateBody(world, obstacle);
  };
  characterFloor = makeObstacle({20, -0.5f, 0}, {10, 0.5f, 10});
  characterWall = makeObstacle({23, 1.5f, 0}, {0.3f, 1.5f, 2});
  characterStep = makeObstacle({18, 0.15f, 2}, {1, 0.15f, 1});
  characterSlope =
      makeObstacle({16, 0.6f, -2}, {1.5f, 0.2f, 1}, QuaternionFromAxisAngle({0, 0, 1}, 1.05f));

  Levye::PhysicsBodyDesc platformDesc{};
  platformDesc.motion = Levye::PhysicsMotionType::Kinematic;
  platformDesc.shape.type = Levye::PhysicsShapeType::Box;
  platformDesc.shape.halfExtents = {2.0f, 0.2f, 2.0f};
  platformDesc.transform.translation = {26.0f, 1.0f, -3.0f};
  platformDesc.layer = PhysicsLayers::World;
  platformDesc.mask = PhysicsLayers::Character;

  characterPlatform = Levye::Physics::CreateBody(world, platformDesc);

  Levye::PhysicsCharacterDesc characterDesc{};
  characterDesc.position = platformTestSpawn;
  characterDesc.layer = PhysicsLayers::Character;
  characterDesc.mask = PhysicsLayers::World;
  character = Levye::Physics::CreateCharacter(world, characterDesc);

  ready = floor.IsValid() && ball.IsValid() && sensor.IsValid() && character.IsValid() &&
          characterFloor.IsValid() && characterWall.IsValid() && characterStep.IsValid() &&
          characterSlope.IsValid() && characterPlatform.IsValid();

  if (!ready) {
    Shutdown();
    return false;
  }

  Levye::Log::Info("Physics: scene initialized");
  return true;
}

void PhysicsScene::Update() {
  if (!ready) return;

  if (Levye::Input::IsPressed("PhysicsCharacterMode")) characterMode = !characterMode;
  if (Levye::Input::IsPressed("PhysicsPause"))
    (void)Levye::Physics::SetWorldPaused(world, !diagnostics.paused);
  if (Levye::Input::IsPressed("PhysicsStep")) (void)Levye::Physics::StepWorldOnce(world);
  if (Levye::Input::IsPressed("PhysicsDebugDraw")) showDebugDraw = !showDebugDraw;

  const float dt = GetFrameTime();

  Transform platformTransform{};

  if (Levye::Physics::GetTransform(world, characterPlatform, platformTransform)) {
    const float x = platformTransform.translation.x;

    if (x >= platformMaxX) {
      platformDirection = -1;
    } else if (x <= platformMinX) {
      platformDirection = 1;
    }

    (void)Levye::Physics::SetLinearVelocity(world, characterPlatform,
                                            {platformSpeed * platformDirection, 0.0f, 0.0f});
  }

  const float horizontal = Levye::Input::GetAxis("PhysicsCameraHorizontal");

  const float vertical = Levye::Input::GetAxis("PhysicsCameraVertical");

  cameraYaw += horizontal * 90.0f * dt;
  cameraPitch += vertical * 90.0f * dt;

  cameraPitch = std::clamp(cameraPitch, -15.0f, 85.0f);

  cameraDistance -= Levye::Input::GetMouseWheel() * 1.5f;
  cameraDistance = std::clamp(cameraDistance, 4.0f, 35.0f);

  if (Levye::Input::IsPressed("PhysicsCameraReset")) {
    cameraYaw = 45.0f;
    cameraPitch = 30.0f;
    cameraDistance = 17.0f;
  }

  if (Levye::Input::IsPressed("PhysicsToggleHUD")) {
    showHUD = !showHUD;
  }

  if (Levye::Input::IsPressed("PhysicsToggleSensor")) {
    const bool next = !sensorFilterEnabled;
    const std::uint16_t mask =
        PhysicsLayers::World | (next ? PhysicsLayers::Trigger : std::uint16_t{0});
    if (Levye::Physics::SetCollisionFilter(world, ball, PhysicsLayers::Ball, mask))
      sensorFilterEnabled = next;
  }
  if (Levye::Input::IsPressed("PhysicsShapeCast")) {
    Levye::PhysicsShapeDesc shape{};
    shape.type = Levye::PhysicsShapeType::Sphere;
    shape.radius = 0.35f;
    Transform start{{-5.0f, 2.5f, 0.0f}, {0, 0, 0, 1}, {1, 1, 1}};
    showCast = Levye::Physics::ShapeCast(world, shape, start, {1, 0, 0}, 10.0f, {}, lastCast);
  }
  if (Levye::Input::IsPressed("PhysicsOverlap")) {
    Levye::PhysicsShapeDesc shape{};
    shape.type = Levye::PhysicsShapeType::Sphere;
    shape.radius = 1.0f;
    Transform center{{0, 2.5f, 0}, {0, 0, 0, 1}, {1, 1, 1}};
    showOverlap = Levye::Physics::OverlapShape(world, shape, center, {}, nullptr, 0, overlapCount);
  }

  Transform transform{};

  if (Levye::Physics::GetTransform(world, ball, transform)) {
    ballPosition = transform.translation;
  }

  if (characterMode) {
    const float speed = Levye::Input::IsDown("PhysicsCharacterSprint") ? 7.0f : 3.5f;
    Vector3 movement{Levye::Input::GetAxis("PhysicsCharacterX"), 0,
                     Levye::Input::GetAxis("PhysicsCharacterZ")};
    if (Vector3LengthSqr(movement) > 1.0f) movement = Vector3Normalize(movement);
    movement = Vector3Scale(movement, speed);
    characterSpeed = Vector3Length(movement);
    (void)Levye::Physics::SetCharacterVelocity(world, character, movement);
    if (Levye::Input::IsPressed("PhysicsCharacterJump"))
      (void)Levye::Physics::JumpCharacter(world, character, 5.0f);
    (void)Levye::Physics::GetCharacterPosition(world, character, characterPosition);
    (void)Levye::Physics::GetCharacterVelocity(world, character, characterVelocity);
    (void)Levye::Physics::GetCharacterGroundState(world, character, characterGroundState);
    (void)Levye::Physics::GetCharacterGroundNormal(world, character, characterGroundNormal);
  } else {
    (void)Levye::Physics::SetCharacterVelocity(world, character, {0, 0, 0});
  }

  if (!characterMode && Levye::Input::IsPressed("PhysicsImpulse")) {
    (void)Levye::Physics::AddImpulse(world, ball, {0.0f, 6.0f, 0.0f});
  }

  bool characterResetThisFrame = false;
  if (Levye::Input::IsPressed("PhysicsReset")) {
    if (characterMode) {
      (void)Levye::Physics::SetCharacterPosition(world, character, platformTestSpawn);
      (void)Levye::Physics::SetCharacterVelocity(world, character, {0, 0, 0});
      characterResetThisFrame = true;
      platformOffsetInitialized = false;
      platformMaxDriftX = 0.0f;
    } else {
      Transform reset{};
      reset.translation = {0.0f, 5.0f, 0.0f};
      reset.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
      reset.scale = {1.0f, 1.0f, 1.0f};

      (void)Levye::Physics::SetTransform(world, ball, reset);
      (void)Levye::Physics::SetLinearVelocity(world, ball, {0.0f, 0.0f, 0.0f});
    }
  }

  if (characterMode && !characterResetThisFrame && characterSpeed < 0.001f &&
      characterGroundState == Levye::PhysicsGroundState::Grounded &&
      Levye::Physics::GetTransform(world, characterPlatform, platformTransform)) {
    const float offsetX = characterPosition.x - platformTransform.translation.x;

    const float offsetZ = characterPosition.z - platformTransform.translation.z;

    // Only sample a resting character supported by the platform's top face.
    const bool abovePlatform =
        std::abs(offsetX) <= 2.0f && std::abs(offsetZ) <= 2.0f &&
        std::abs(characterPosition.y - (platformTransform.translation.y + 0.2f + 0.9f)) < 0.15f;

    if (abovePlatform) {
      if (!platformOffsetInitialized) {
        platformInitialOffsetX = offsetX;
        platformMaxDriftX = 0.0f;
        platformOffsetInitialized = true;
      }

      platformMaxDriftX = std::max(platformMaxDriftX, std::abs(offsetX - platformInitialOffsetX));
    } else {
      platformOffsetInitialized = false;
    }
  } else {
    platformOffsetInitialized = false;
  }

  Levye::PhysicsContactEvent event{};

  while (Levye::Physics::PollContactEvent(world, event)) {
    if (event.type != Levye::PhysicsContactEventType::End) {
      debugContactPosition = event.position;
      debugContactNormal = event.normal;
      haveDebugContact = true;
    }
    switch (event.type) {
      case Levye::PhysicsContactEventType::Begin:
        ++contactBegins;
        break;

      case Levye::PhysicsContactEventType::Persist:
        ++contactPersists;
        break;

      case Levye::PhysicsContactEventType::End:
        ++contactEnds;
        break;
    }

    const bool isBallSensorPair = (event.bodyA == ball && event.bodyB == sensor) ||
                                  (event.bodyA == sensor && event.bodyB == ball);

    if (!event.isSensor || !isBallSensorPair) {
      continue;
    }

    switch (event.type) {
      case Levye::PhysicsContactEventType::Begin:
        if (!ballInsideSensor) {
          ballInsideSensor = true;
          ++sensorEnterCount;
          Levye::Log::Info("Physics: Ball entered sensor");
        }
        break;

      case Levye::PhysicsContactEventType::Persist:
        break;

      case Levye::PhysicsContactEventType::End:
        if (ballInsideSensor) {
          ballInsideSensor = false;
          ++sensorExitCount;
          Levye::Log::Info("Physics: Ball exited sensor");
        }
        break;
    }
  }
  (void)Levye::Physics::GetWorldDiagnostics(world, diagnostics);
}

void PhysicsScene::Draw() const {
  ClearBackground({18, 22, 32, 255});

  Camera3D camera{};
  constexpr float DEG_TO_RAD = 0.017453292519943295f;

  const float yaw = cameraYaw * DEG_TO_RAD;
  const float pitch = cameraPitch * DEG_TO_RAD;

  const Vector3 target =
      characterMode ? Vector3{characterPosition.x, characterPosition.y + 0.6f, characterPosition.z}
                    : Vector3{0.0f, 1.5f, 0.0f};

  const float horizontalDistance = cameraDistance * std::cos(pitch);

  camera.position = {target.x + horizontalDistance * std::sin(yaw),
                     target.y + cameraDistance * std::sin(pitch),
                     target.z + horizontalDistance * std::cos(yaw)};

  camera.target = target;
  camera.up = {0.0f, 1.0f, 0.0f};
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  BeginMode3D(camera);

  DrawGrid(20, 1.0f);

  DrawCube({0.0f, -0.5f, 0.0f}, 20.0f, 1.0f, 20.0f, DARKGRAY);

  const Vector3 sensorPosition{0.0f, 2.5f, 0.0f};

  const Color sensorColor = ballInsideSensor ? GREEN : SKYBLUE;

  if (ready) {
    DrawSphere(ballPosition, 0.5f, ORANGE);
    DrawCubeWires(sensorPosition, 4.0f, 1.5f, 4.0f, sensorColor);
    DrawLine3D({-5, 2.5f, 0}, {5, 2.5f, 0}, YELLOW);
    if (showCast) DrawSphere(lastCast.position, 0.12f, YELLOW);
    if (showOverlap) DrawSphereWires({0, 2.5f, 0}, 1.0f, 12, 12, PURPLE);
    if (characterMode) {
      DrawCube({20, -0.5f, 0}, 20, 1, 20, DARKGRAY);
      DrawCube({23, 1.5f, 0}, 0.6f, 3, 4, MAROON);
      DrawCube({18, 0.15f, 2}, 2, 0.3f, 2, GREEN);
      Transform platformTransform{};

      if (Levye::Physics::GetTransform(world, characterPlatform, platformTransform)) {
        DrawCube(platformTransform.translation, 4.0f, 0.4f, 4.0f, BLUE);
      }
      rlPushMatrix();
      rlTranslatef(16, 0.6f, -2);
      rlRotatef(1.05f * RAD2DEG, 0, 0, 1);
      DrawCube({0, 0, 0}, 3, 0.4f, 2, ORANGE);
      rlPopMatrix();
      DrawCapsule({characterPosition.x, characterPosition.y - 0.55f, characterPosition.z},
                  {characterPosition.x, characterPosition.y + 0.55f, characterPosition.z}, 0.35f, 8,
                  8, YELLOW);
    }
    if (showDebugDraw) {
      if (haveDebugContact) {
        DrawSphere(debugContactPosition, 0.12f, MAGENTA);
        DrawLine3D(debugContactPosition,
                   Vector3Add(debugContactPosition, Vector3Scale(debugContactNormal, 0.8f)),
                   MAGENTA);
      }
      if (characterMode && characterGroundState != Levye::PhysicsGroundState::Airborne)
        DrawLine3D(
            {characterPosition.x, characterPosition.y - 0.9f, characterPosition.z},
            Vector3Add({characterPosition.x, characterPosition.y - 0.9f, characterPosition.z},
                       characterGroundNormal),
            LIME);
    }
  }

  EndMode3D();

  if (characterMode) {
    if (showHUD) {
      const char* state = characterGroundState == Levye::PhysicsGroundState::Grounded  ? "GROUNDED"
                          : characterGroundState == Levye::PhysicsGroundState::Sliding ? "SLIDING"
                                                                                       : "AIRBORNE";
      DrawRectangle(10, 10, 560, 250, Fade(BLACK, 0.75f));
      DrawText("PHYSICS CHARACTER | TAB: Rigid Bodies", 20, 20, 20, WHITE);
      DrawText("WASD: Move | SHIFT: Sprint | SPACE: Jump | R: Reset", 20, 48, 16, LIGHTGRAY);
      DrawText(TextFormat("Position %.2f %.2f %.2f | Speed %.1f", characterPosition.x,
                          characterPosition.y, characterPosition.z, characterSpeed),
               20, 75, 17, WHITE);
      DrawText(TextFormat("Velocity %.2f %.2f %.2f | %s", characterVelocity.x, characterVelocity.y,
                          characterVelocity.z, state),
               20, 100, 17, WHITE);
      DrawText(
          TextFormat("Ground normal %.2f %.2f %.2f | Step 0.30 | Slope 45 deg",
                     characterGroundNormal.x, characterGroundNormal.y, characterGroundNormal.z),
          20, 125, 17, LIGHTGRAY);
      DrawText("ARROWS: Orbit | WHEEL: Zoom | H: HUD | ESC: Menu", 20, 150, 15, LIGHTGRAY);

      DrawText(platformOffsetInitialized
                   ? TextFormat("Idle platform drift X: %.4f", platformMaxDriftX)
                   : "Idle platform drift: waiting for support",
               20, 205, 17, YELLOW);
      DrawText(TextFormat("P: %s | .: Step | B: Debug %s | Bodies %u Contacts %u",
                          diagnostics.paused ? "Resume" : "Pause", showDebugDraw ? "ON" : "OFF",
                          diagnostics.bodyCount, diagnostics.activeContactPairs),
               20, 230, 15, LIGHTGRAY);

      Transform platformTransform{};

      if (Levye::Physics::GetTransform(world, characterPlatform, platformTransform)) {
        const Vector3 relative = Vector3Subtract(characterPosition, platformTransform.translation);

        DrawText(TextFormat("Platform offset X: %.3f | Z: %.3f", relative.x, relative.z), 20, 180,
                 17, YELLOW);
      }
    }
    return;
  }

  if (showHUD) {
    DrawRectangle(10, 10, 620, 350, Fade(BLACK, 0.75f));

    DrawText("LEVYEKIT PHYSICS", 20, 20, 30, RAYWHITE);

    DrawText("SPACE: Impulse | R: Reset | ESC: Menu", 20, 60, 18, LIGHTGRAY);

    DrawText("ARROWS: Orbit | WHEEL: Zoom | C: Reset Camera", 20, 85, 18, LIGHTGRAY);

    DrawText(TextFormat("Ball Y: %.3f", ballPosition.y), 20, 115, 20, WHITE);

    DrawText(TextFormat("Hot reloads: %d", reloadCount), 20, 140, 20, GREEN);

    DrawText(TextFormat("Contacts: Begin %d | Persist %d | End %d", contactBegins, contactPersists,
                        contactEnds),
             20, 165, 20, LIGHTGRAY);

    DrawText(
        TextFormat("Trigger: %s | Enter %d | Exit %d", ballInsideSensor ? "ACTIVE" : "INACTIVE",
                   sensorEnterCount, sensorExitCount),
        20, 190, 20, ballInsideSensor ? GREEN : SKYBLUE);

    DrawText(TextFormat("F: Sensor filter %s | Layer 0x%04X Mask 0x%04X",
                        sensorFilterEnabled ? "ON" : "OFF", PhysicsLayers::Ball,
                        PhysicsLayers::World | (sensorFilterEnabled ? PhysicsLayers::Trigger : 0)),
             20, 220, 17, LIGHTGRAY);
    DrawText(TextFormat("G: Cast %s (%.2f) | O: Overlap %u", showCast ? "HIT" : "MISS",
                        lastCast.fraction, overlapCount),
             20, 245, 17, YELLOW);

    DrawText("H: Hide HUD", 20, 275, 16, GRAY);
    DrawText(TextFormat("P: %s | .: Step | B: Debug %s", diagnostics.paused ? "Resume" : "Pause",
                        showDebugDraw ? "ON" : "OFF"),
             20, 300, 16, LIGHTGRAY);
    DrawText(TextFormat("Bodies %u | Characters %u | Contacts %u | Step %.3f ms",
                        diagnostics.bodyCount, diagnostics.characterCount,
                        diagnostics.activeContactPairs, diagnostics.lastStepMilliseconds),
             20, 325, 15, LIGHTGRAY);
  } else {
    DrawText("H: Show HUD", 20, 20, 18, LIGHTGRAY);
  }

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
  reloadSnapshotValid &=
      Levye::Physics::GetCharacterPosition(world, character, characterBeforeReload);
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

  Vector3 characterAfterReload{};
  const bool sameCharacter =
      Levye::Physics::GetCharacterPosition(world, character, characterAfterReload) &&
      Vector3Distance(characterAfterReload, characterBeforeReload) < tolerance;
  return samePosition && sameVelocity && sameCharacter;
}

void PhysicsScene::Shutdown() {
  if (world.IsValid()) {
    (void)Levye::Physics::DestroyWorld(world);
  }

  world = {};
  floor = {};
  ball = {};
  sensor = {};
  character = {};
  characterFloor = {};
  characterWall = {};
  characterStep = {};
  characterSlope = {};
  characterPlatform = {};
  characterMode = false;
  ballInsideSensor = false;
  sensorFilterEnabled = true;
  showCast = false;
  showOverlap = false;
  overlapCount = 0;
  platformDirection = 1;
  ready = false;
  reloadSnapshotValid = false;
  platformOffsetInitialized = false;
  platformInitialOffsetX = 0.0f;
  platformMaxDriftX = 0.0f;
  diagnostics = {};
  haveDebugContact = false;
}

}  // namespace Sandbox

#endif
