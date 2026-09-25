#include <Levye/Core/Application.hpp>
#include <Levye/HotReload/GameModule.hpp>

#include <raylib.h>

#include <iostream>
#include <string>

int main() {
#if defined(__APPLE__)

  const std::string gameLibraryPath = "Targets/Debug/lib/Game.dylib";

#elif defined(__linux__)

  const std::string gameLibraryPath = "Targets/Debug/lib/Game.so";

#elif defined(_WIN32)

  const std::string gameLibraryPath = "Targets/Debug/lib/Game.dll";

#else

#error Unsupported platform

#endif

  Levye::GameModule gameModule;
  gameModule.GetScreenManager().SetScreen("Menu");

  auto &input = gameModule.GetInputMap();
  /*
   * Keyboard movement.
   *
   * Both WASD and arrow keys feed the same logical axes.
   */
  input.BindKeyAxis("MoveX", KEY_A, KEY_D);

  input.BindKeyAxis("MoveX", KEY_LEFT, KEY_RIGHT);

  input.BindKeyAxis("MoveY", KEY_W, KEY_S);

  input.BindKeyAxis("MoveY", KEY_UP, KEY_DOWN);

  /*
   * Controller movement.
   */
  input.BindGamepadAxis("MoveX", 0, GAMEPAD_AXIS_LEFT_X);

  input.BindGamepadAxis("MoveY", 0, GAMEPAD_AXIS_LEFT_Y);

  input.BindKey("MoveUp", KEY_W);

  input.BindKey("MoveUp", KEY_UP);

  input.BindKey("MoveDown", KEY_S);

  input.BindKey("MoveDown", KEY_DOWN);

  input.BindKey("MoveLeft", KEY_A);

  input.BindKey("MoveLeft", KEY_LEFT);

  input.BindKey("MoveRight", KEY_D);

  input.BindKey("MoveRight", KEY_RIGHT);

  input.BindKey("Confirm", KEY_ENTER);

  input.BindKey("Back", KEY_ESCAPE);

  input.BindKey("TestSound", KEY_SPACE);
  input.BindGamepadButton("TestSound", 0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);

  input.BindKey("PauseMusic", KEY_P);

  input.BindKey("ResumeMusic", KEY_R);

  input.BindKey("NormalTime", KEY_ONE);

  input.BindKey("SlowTime", KEY_TWO);

  input.BindKey("FastTime", KEY_THREE);

  input.BindKey("Pause", KEY_P);

  if (!gameModule.Load(gameLibraryPath)) {
    std::cerr << "[Sandbox] Failed to load game module.\n";

    return 1;
  }

  Levye::ApplicationConfig config;

  config.width = 1280;
  config.height = 720;
  config.title = "LevyeKit Sandbox";
  config.targetFPS = 60;

  Levye::Application app(config, gameModule);

  app.Run();

  return 0;
}