#include "MenuScene.hpp"

#include <Levye/LevyeKit.hpp>
namespace Sandbox {
void MenuScene::Update() {
  if (Levye::Input::IsPressed("Confirm")) Levye::Screen::Set("Game");
}
void MenuScene::Draw(bool animationReady) {
  ClearBackground(BLACK);

  DrawText("LEVYEKIT", 40, 40, 40, RAYWHITE);

  DrawText("Press ENTER / Controller A to play", 40, 100, 24, LIGHTGRAY);

#ifdef LEVYE_WITH_ANIMATION
  if (animationReady) {
    DrawText("T: Animation scenes | F1-F11: Open a scene", 40, 135, 24, LIGHTGRAY);
  }

#endif
#ifdef LEVYE_WITH_PHYSICS
  DrawText("F12: Physics test scene", 40, 170, 24, LIGHTGRAY);
#endif
}
}  // namespace Sandbox
