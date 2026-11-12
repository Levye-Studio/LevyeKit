#include "InputScene.hpp"

#include <Levye/LevyeKit.hpp>
#include <string>
namespace Sandbox {
void InputScene::Bind() {
  /*
   * Keyboard movement.
   *
   * Both WASD and arrow keys feed the same logical axes.
   */
  Levye::Input::BindKeyAxis("MoveX", KEY_A, KEY_D);

  Levye::Input::BindKeyAxis("MoveX", KEY_LEFT, KEY_RIGHT);

  Levye::Input::BindKeyAxis("MoveY", KEY_W, KEY_S);

  Levye::Input::BindKeyAxis("MoveY", KEY_UP, KEY_DOWN);

  /*
   * Controller movement.
   */
  Levye::Input::BindGamepadAxis("MoveX", 0, GAMEPAD_AXIS_LEFT_X);

  Levye::Input::BindGamepadAxis("MoveY", 0, GAMEPAD_AXIS_LEFT_Y);

  Levye::Input::BindKey("MoveUp", KEY_W);

  Levye::Input::BindKey("MoveUp", KEY_UP);

  Levye::Input::BindKey("MoveDown", KEY_S);

  Levye::Input::BindKey("MoveDown", KEY_DOWN);

  Levye::Input::BindKey("MoveLeft", KEY_A);

  Levye::Input::BindKey("MoveLeft", KEY_LEFT);

  Levye::Input::BindKey("MoveRight", KEY_D);

  Levye::Input::BindKey("MoveRight", KEY_RIGHT);

  Levye::Input::BindKey("Confirm", KEY_ENTER);
  Levye::Input::BindGamepadButton("Confirm", 0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);

  Levye::Input::BindKey("Back", KEY_ESCAPE);
  Levye::Input::BindGamepadButton("Back", 0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);

  Levye::Input::BindKey("TestSound", KEY_SPACE);
  Levye::Input::BindGamepadButton("TestSound", 0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);

  Levye::Input::BindKey("PauseMusic", KEY_M);

  Levye::Input::BindKey("ResumeMusic", KEY_R);

  Levye::Input::BindKey("NormalTime", KEY_ONE);

  Levye::Input::BindKey("SlowTime", KEY_TWO);

  Levye::Input::BindKey("FastTime", KEY_THREE);

  Levye::Input::BindKey("Pause", KEY_P);

#ifdef LEVYE_WITH_ANIMATION
  Levye::Input::BindKey("ShowAnimation", KEY_T);
  Levye::Input::BindKey("AnimationScene1", KEY_F1);
  Levye::Input::BindKey("AnimationScene2", KEY_F2);
  Levye::Input::BindKey("AnimationScene3", KEY_F3);
  Levye::Input::BindKey("AnimationScene4", KEY_F4);
  Levye::Input::BindKey("AnimationScene5", KEY_F5);
  Levye::Input::BindKey("AnimationScene6", KEY_F6);
  Levye::Input::BindKey("AnimationScene7", KEY_F7);
  Levye::Input::BindKey("AnimationNext", KEY_RIGHT);
  Levye::Input::BindKey("AnimationPrevious", KEY_LEFT);
  Levye::Input::BindKey("AnimationConfirm", KEY_ENTER);
  Levye::Input::BindKey("AnimationForce", KEY_F);
  Levye::Input::BindKey("AnimationPause", KEY_P);
  Levye::Input::BindKey("AnimationFaster", KEY_UP);
  Levye::Input::BindKey("AnimationSlower", KEY_DOWN);
  Levye::Input::BindKey("AnimationMove", KEY_W);
  Levye::Input::BindKey("AnimationRun", KEY_LEFT_SHIFT);
  Levye::Input::BindKey("AnimationDance", KEY_C);
  Levye::Input::BindKey("AnimationPunch", KEY_SPACE);
  Levye::Input::BindKey("AnimationJump", KEY_J);
  Levye::Input::BindKey("AnimationRestart", KEY_R);
  Levye::Input::BindKey("AnimationBoneNext", KEY_RIGHT_BRACKET);
  Levye::Input::BindKey("AnimationBonePrevious", KEY_LEFT_BRACKET);
  Levye::Input::BindKey("AnimationBonePositive", KEY_E);
  Levye::Input::BindKey("AnimationBoneNegative", KEY_Q);
  Levye::Input::BindKey("AnimationBoneReset", KEY_B);
  Levye::Input::BindKey("AnimationLeft", KEY_A);
  Levye::Input::BindKey("AnimationRight", KEY_D);
  Levye::Input::BindKey("AnimationDeath", KEY_D);
  Levye::Input::BindKey("AnimationWave", KEY_V);
  Levye::Input::BindKey("AnimationUnload", KEY_U);
  Levye::Input::BindKey("AnimationVerify", KEY_V);
  Levye::Input::BindKey("AnimationScene8", KEY_F8);
  Levye::Input::BindKey("AnimationScene9", KEY_F9);
  Levye::Input::BindKey("AnimationScene10", KEY_F10);
  Levye::Input::BindKey("AnimationScene11", KEY_F11);
  Levye::Input::BindKey("OrientationFlat", KEY_ONE);
  Levye::Input::BindKey("OrientationSlope", KEY_TWO);
  Levye::Input::BindKey("OrientationRotate", KEY_THREE);
  Levye::Input::BindKey("OrientationWeightDown", KEY_LEFT_BRACKET);
  Levye::Input::BindKey("OrientationWeightUp", KEY_RIGHT_BRACKET);
  Levye::Input::BindKey("OrientationWeightZero", KEY_ZERO);
  Levye::Input::BindKey("OrientationWeightFull", KEY_NINE);
  Levye::Input::BindKey("OrientationConstraints", KEY_C);
  Levye::Input::BindKey("OrientationFreeze", KEY_F);
  Levye::Input::BindKey("LocomotionBiped", KEY_ONE);
  Levye::Input::BindKey("LocomotionQuadruped", KEY_TWO);
  Levye::Input::BindKey("LocomotionHexapod", KEY_THREE);
  Levye::Input::BindKey("LocomotionWalk", KEY_G);
  Levye::Input::BindKey("LocomotionPause", KEY_P);
  Levye::Input::BindKey("LocomotionLeft", KEY_A);
  Levye::Input::BindKey("LocomotionRight", KEY_D);
  Levye::Input::BindKey("LocomotionForward", KEY_W);
  Levye::Input::BindKey("LocomotionBack", KEY_S);
  Levye::Input::BindKey("LocomotionTurnLeft", KEY_Q);
  Levye::Input::BindKey("LocomotionTurnRight", KEY_E);
  Levye::Input::BindKey("LocomotionSpeedDown", KEY_MINUS);
  Levye::Input::BindKey("LocomotionSpeedUp", KEY_EQUAL);
  Levye::Input::BindKey("LocomotionStrideDown", KEY_COMMA);
  Levye::Input::BindKey("LocomotionStrideUp", KEY_PERIOD);
  Levye::Input::BindKey("LocomotionMoveSpeedDown", KEY_SEMICOLON);
  Levye::Input::BindKey("LocomotionMoveSpeedUp", KEY_APOSTROPHE);
  Levye::Input::BindKey("LocomotionStabilization", KEY_B);
  Levye::Input::BindKey("LocomotionStrengthDown", KEY_N);
  Levye::Input::BindKey("LocomotionStrengthUp", KEY_H);
  Levye::Input::BindKey("LocomotionSupportDebug", KEY_V);
  Levye::Input::BindKey("LocomotionMarch", KEY_M);
  Levye::Input::BindKey("LocomotionHeightDown", KEY_LEFT_BRACKET);
  Levye::Input::BindKey("LocomotionHeightUp", KEY_RIGHT_BRACKET);
  Levye::Input::BindKey("ContactSelect", KEY_TAB);
  Levye::Input::BindKey("ContactToggle", KEY_C);
  Levye::Input::BindKey("ContactRelease", KEY_SPACE);
  Levye::Input::BindKey("ContactSurfacePrevious", KEY_LEFT);
  Levye::Input::BindKey("ContactSurfaceNext", KEY_RIGHT);

  Levye::Input::BindKey("IKToggle", KEY_I);
  Levye::Input::BindKey("IKTargetLeft", KEY_J);
  Levye::Input::BindKey("IKTargetRight", KEY_L);
  Levye::Input::BindKey("IKTargetForward", KEY_U);
  Levye::Input::BindKey("IKTargetBackward", KEY_O);
  Levye::Input::BindKey("IKTargetUp", KEY_K);
  Levye::Input::BindKey("IKTargetDown", KEY_M);
  Levye::Input::BindKey("IKPoleReverse", KEY_P);
  Levye::Input::BindKey("IKReset", KEY_R);
  Levye::Input::BindKey("IKWeightDown", KEY_LEFT_BRACKET);
  Levye::Input::BindKey("IKWeightUp", KEY_RIGHT_BRACKET);
  Levye::Input::BindKey("IKResponseDown", KEY_MINUS);
  Levye::Input::BindKey("IKResponseUp", KEY_EQUAL);
  Levye::Input::BindKey("IKConstraintToggle", KEY_ONE);
  Levye::Input::BindKey("IKMinDown", KEY_TWO);
  Levye::Input::BindKey("IKMinUp", KEY_THREE);
  Levye::Input::BindKey("IKMaxDown", KEY_FOUR);
  Levye::Input::BindKey("IKMaxUp", KEY_FIVE);
  Levye::Input::BindKey("IKSelectChain", KEY_TAB);
#endif
#ifdef LEVYE_WITH_PHYSICS
  Levye::Input::BindKey("ShowPhysics", KEY_GRAVE);
  Levye::Input::BindKey("PhysicsImpulse", KEY_SPACE);
  Levye::Input::BindKey("PhysicsReset", KEY_R);
  Levye::Input::BindKeyAxis("PhysicsCameraHorizontal", KEY_LEFT, KEY_RIGHT);
  Levye::Input::BindKeyAxis("PhysicsCameraVertical", KEY_DOWN, KEY_UP);
  Levye::Input::BindKey("PhysicsCameraReset", KEY_C);
  Levye::Input::BindKey("PhysicsToggleHUD", KEY_H);
  Levye::Input::BindKey("PhysicsPause", KEY_P);
  Levye::Input::BindKey("PhysicsStep", KEY_PERIOD);
  Levye::Input::BindKey("PhysicsDebugDraw", KEY_B);
  Levye::Input::BindKey("PhysicsToggleSensor", KEY_F);
  Levye::Input::BindKey("PhysicsShapeCast", KEY_G);
  Levye::Input::BindKey("PhysicsOverlap", KEY_O);
  Levye::Input::BindKey("PhysicsCharacterMode", KEY_TAB);
  Levye::Input::BindKeyAxis("PhysicsCharacterX", KEY_A, KEY_D);
  Levye::Input::BindKeyAxis("PhysicsCharacterZ", KEY_W, KEY_S);
  Levye::Input::BindKey("PhysicsCharacterJump", KEY_SPACE);
  Levye::Input::BindKey("PhysicsCharacterSprint", KEY_LEFT_SHIFT);

#endif

  Levye::Input::BindKey("TestMouse", KEY_C);
  Levye::Input::BindMouseButton("TestMouse", MOUSE_BUTTON_LEFT);
  Levye::Input::BindKey("DisplayCycleScaling", KEY_SPACE);
}
void InputScene::Update() {
  if (Levye::Input::IsPressed("DisplayCycleScaling") && Levye::Display::IsEnabled()) {
    const auto current = Levye::Display::GetScaling();
    const auto next = current == Levye::DisplayScaling::Fit    ? Levye::DisplayScaling::Fill
                      : current == Levye::DisplayScaling::Fill ? Levye::DisplayScaling::Stretch
                                                               : Levye::DisplayScaling::Fit;
    (void)Levye::Display::SetScaling(next);
  }
  if (Levye::Input::IsPressed("TestMouse")) {
    Levye::Log::Info("Mouse pressed");
  }

  if (Levye::Input::IsReleased("TestMouse")) {
    Levye::Log::Info("Mouse released");
  }

  const float mouseWheel = Levye::Input::GetMouseWheel();
  const Vector2 mouseDelta = Levye::Input::GetMouseDelta();

  if (mouseWheel != 0.0f) {
    std::string wheelMSG = "Wheel: " + std::to_string(mouseWheel);
    Levye::Log::Info(wheelMSG.c_str());
  }

  if (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f) {
    std::string mouseMSG =
        "Mouse delta: x=" + std::to_string(mouseDelta.x) + " y=" + std::to_string(mouseDelta.y);
    Levye::Log::Info(mouseMSG.c_str());
  }
}

void InputScene::Draw() {
  Levye::DisplayInfo info;
  if (Levye::Display::GetInfo(info)) {
    const char* scaling = info.scaling == Levye::DisplayScaling::Fit    ? "Fit"
                          : info.scaling == Levye::DisplayScaling::Fill ? "Fill"
                                                                        : "Stretch";
    DrawText(TextFormat("SPACE: %s | logical %dx%d | window %dx%d | framebuffer %dx%d", scaling,
                        info.logical.width, info.logical.height, info.window.width,
                        info.window.height, info.framebuffer.width, info.framebuffer.height),
             40, 130, 18, RAYWHITE);
    DrawText(TextFormat("Viewport fb: (%.1f, %.1f) %.1f x %.1f", info.viewport.x, info.viewport.y,
                        info.viewport.width, info.viewport.height),
             40, 154, 18, RAYWHITE);
    DrawRectangleLines(0, 0, info.logical.width - 1, info.logical.height - 1, LIME);
    const Vector2 raw = GetMousePosition();
    const Vector2 logical = Levye::Input::GetMousePosition();
    DrawText(TextFormat("Mouse raw (%.1f, %.1f) logical (%.1f, %.1f) %s", raw.x, raw.y, logical.x,
                        logical.y, Levye::Input::IsMouseInsideViewport() ? "inside" : "outside"),
             40, 178, 18, RAYWHITE);
    DrawCircleLines(info.logical.width * 3 / 4, info.logical.height / 2, 25.0f, YELLOW);
    DrawLine(info.logical.width * 3 / 4 - 35, info.logical.height / 2,
             info.logical.width * 3 / 4 + 35, info.logical.height / 2, YELLOW);
    DrawLine(info.logical.width * 3 / 4, info.logical.height / 2 - 35, info.logical.width * 3 / 4,
             info.logical.height / 2 + 35, YELLOW);
    if (Levye::Input::IsMouseInsideViewport())
      DrawCircleLines(static_cast<int>(logical.x), static_cast<int>(logical.y), 12.0f, MAGENTA);
  }
  DrawText(
      TextFormat("Touches: %d (hold left mouse to simulate one)", Levye::Input::GetTouchCount()),
      40, 205, 18, RAYWHITE);
  for (int i = 0; i < Levye::Input::GetTouchEventCount(); ++i) {
    Levye::TouchPoint touch;
    if (!Levye::Input::GetTouchEvent(i, touch)) continue;
    const bool released = Levye::Input::IsTouchReleased(touch);
    const Color color = released                                  ? ORANGE
                        : touch.phase == Levye::TouchPhase::Began ? GREEN
                                                                  : SKYBLUE;
    DrawCircleLines(static_cast<int>(touch.x), static_cast<int>(touch.y), 24.0f, color);
    DrawText(TextFormat("%d %s", touch.id,
                        touch.phase == Levye::TouchPhase::Cancelled ? "cancelled"
                        : touch.phase == Levye::TouchPhase::Ended   ? "ended"
                        : touch.phase == Levye::TouchPhase::Began   ? "began"
                        : touch.phase == Levye::TouchPhase::Moved   ? "moved"
                                                                    : "stationary"),
             static_cast<int>(touch.x) + 28, static_cast<int>(touch.y) - 10, 16, color);
  }
}
}  // namespace Sandbox
