
#pragma once

#ifdef LEVYE_WITH_PHYSICS

#include <Levye/Modules/Physics/Physics.hpp>

namespace Sandbox {

/**
 * @brief Sandbox scene for testing the optional Physics module.
 *
 * World and body handles survive game-module hot reloads.
 * The underlying simulation remains owned by the host.
 */
struct PhysicsScene {
  Levye::PhysicsWorldHandle world{};
  Levye::PhysicsBodyHandle floor{};
  Levye::PhysicsBodyHandle ball{};
  Levye::PhysicsBodyHandle sensor{};
  Levye::PhysicsCharacterHandle character{};
  Levye::PhysicsBodyHandle characterFloor{};
  Levye::PhysicsBodyHandle characterWall{};
  Levye::PhysicsBodyHandle characterStep{};
  Levye::PhysicsBodyHandle characterSlope{};
  Levye::PhysicsBodyHandle characterPlatform{};
  Vector3 characterPosition{20, 2, 0};
  Vector3 characterVelocity{};
  Vector3 characterGroundNormal{};
  Levye::PhysicsGroundState characterGroundState = Levye::PhysicsGroundState::Airborne;
  bool characterMode = false;
  float characterSpeed = 0;

  Vector3 ballPosition{0.0f, 5.0f, 0.0f};
  Vector3 velocityBeforeReload{};
  Vector3 positionBeforeReload{};
  Vector3 characterBeforeReload{};

  int contactBegins = 0;
  int contactPersists = 0;
  int contactEnds = 0;
  int sensorEnterCount = 0;
  int sensorExitCount = 0;

  float cameraYaw = 45.0f;
  float cameraPitch = 30.0f;
  float cameraDistance = 17.0f;

  float platformSpeed = 1.5f;
  float platformMinX = 24.0f;
  float platformMaxX = 32.0f;
  int platformDirection = 1;

  float platformInitialOffsetX = 0.0f;
  float platformMaxDriftX = 0.0f;
  bool platformOffsetInitialized = false;

  bool showHUD = true;
  bool showDebugDraw = true;
  bool haveDebugContact = false;
  Vector3 debugContactPosition{};
  Vector3 debugContactNormal{};
  Levye::PhysicsWorldDiagnostics diagnostics{};

  int reloadCount = 0;
  bool ready = false;
  bool reloadSnapshotValid = false;
  bool ballInsideSensor = false;
  bool sensorFilterEnabled = true;
  bool showCast = false;
  bool showOverlap = false;
  Levye::PhysicsShapeCastHit lastCast{};
  std::uint32_t overlapCount = 0;

  [[nodiscard]] bool Initialize();
  void Update();
  void Draw() const;
  void BeforeReload();
  [[nodiscard]] bool AfterReload();
  void Shutdown();
};

}  // namespace Sandbox

#endif
