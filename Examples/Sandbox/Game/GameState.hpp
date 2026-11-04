#pragma once
#include <Levye/LevyeKit.hpp>
#include <Levye/Modules/Serialization/Serialization.hpp>

#include "Scenes/Core/CoreScene.hpp"
#ifdef LEVYE_WITH_ANIMATION
#include "Scenes/Animation/AnimationScene.hpp"
#include "Scenes/Animation/AnimationTest.hpp"
#endif
#ifdef LEVYE_WITH_PHYSICS
#include "Scenes/Physics/PhysicsScene.hpp"
#endif
namespace Sandbox {
/** @brief Host-allocated state retained across reloadable Game library swaps. */
struct GameState {
  CoreScene core;
  Levye::DocumentHandle persistentDocument = Levye::InvalidDocumentHandle;
#ifdef LEVYE_WITH_ANIMATION
  AnimationTest animationTest;
  AnimationScene animationScene;
#endif
#ifdef LEVYE_WITH_PHYSICS
  PhysicsScene physicsScene;
#endif
};
}  // namespace Sandbox
