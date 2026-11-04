
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

  Vector3 ballPosition{0.0f, 5.0f, 0.0f};
  Vector3 velocityBeforeReload{};
  Vector3 positionBeforeReload{};

  int reloadCount = 0;
  bool ready = false;
  bool reloadSnapshotValid = false;

  [[nodiscard]] bool Initialize();
  void Update();
  void Draw() const;
  void BeforeReload();
  [[nodiscard]] bool AfterReload();
  void Shutdown();
};

}  // namespace Sandbox

#endif
