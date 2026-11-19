#include "MenuScene.hpp"

#include <Levye/LevyeKit.hpp>
namespace Sandbox {
void MenuScene::Update() {
  if (Levye::Input::IsPressed("Confirm")) Levye::Screen::Set("Game");
#if defined(__ANDROID__)
  for (int index = 0; index < Levye::Input::GetTouchEventCount(); ++index) {
    Levye::TouchPoint touch;
    if (Levye::Input::GetTouchEvent(index, touch) && touch.insideViewport &&
        Levye::Input::IsTouchPressed(touch) && touch.y < 260.0f)
      Levye::Screen::Set("Game");
  }
#endif
}
void MenuScene::Draw(bool animationReady) {
  ClearBackground(BLACK);

  DrawText("LEVYEKIT", 40, 40, 40, RAYWHITE);

  DrawText("Press ENTER / Controller A to play", 40, 100, 24, LIGHTGRAY);
#if defined(__ANDROID__)
  DrawText("Tap here to play", 40, 150, 26, GOLD);
  Levye::DisplayInfo info;
  if (Levye::Display::GetInfo(info)) {
    DrawText(TextFormat("Touch: %d  Events: %d", Levye::Input::GetTouchCount(),
                        Levye::Input::GetTouchEventCount()),
             40, 225, 20, RAYWHITE);
    DrawText(TextFormat("Logical %dx%d  Window %dx%d  Framebuffer %dx%d", info.logical.width,
                        info.logical.height, info.window.width, info.window.height,
                        info.framebuffer.width, info.framebuffer.height),
             40, 260, 16, LIGHTGRAY);
    DrawText(TextFormat("Fit viewport %.0f,%.0f %.0fx%.0f", info.viewport.x, info.viewport.y,
                        info.viewport.width, info.viewport.height),
             40, 285, 16, LIGHTGRAY);
  }
#endif

#ifdef LEVYE_WITH_ANIMATION
  if (animationReady) {
    DrawText("T: Animation scenes | F1-F11: Open a scene", 40, 135, 24, LIGHTGRAY);
  }

#endif
#ifdef LEVYE_WITH_PHYSICS
  DrawText("`: Physics test scene", 40, 170, 24, LIGHTGRAY);
#endif
}
}  // namespace Sandbox
