#include <Levye/Core/Application.hpp>
#include <Levye/HotReload/GameModule.hpp>
#include <Levye/Platform/ExecutablePath.hpp>
#include <Levye/Platform/SharedLibrary.hpp>
#include <Levye/Project/ProjectConfig.hpp>
#include <Levye/Project/ProjectLoader.hpp>

#include <raylib.h>

#include <iostream>
#include <string>

int main() {

  const std::filesystem::path executableDirectory =
      Levye::ExecutablePath::GetDirectory();

  const std::filesystem::path targetDirectory =
      executableDirectory.parent_path();

  const std::filesystem::path gameModulePath =
      targetDirectory / "lib" / Levye::SharedLibrary::MakeFilename("Game");

  const std::filesystem::path frameworkRoot =
      (executableDirectory / "../../..").lexically_normal();

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

  const std::filesystem::path projectFile =
      frameworkRoot / "Examples/Sandbox/levye.project";

  const std::filesystem::path projectDirectory = projectFile.parent_path();

  const auto project = Levye::ProjectLoader::Load(projectFile);

  if (!project) {
    std::cerr << "[Sandbox] Failed to load project configuration.\n";

    return 1;
  }

  const std::filesystem::path assetRoot =
      projectDirectory / project->assetDirectory;

  Levye::ApplicationConfig config = project->CreateApplicationConfig();

  gameModule.SetAssetRoot(assetRoot.string());

  if (!gameModule.Load(gameModulePath.string())) {
    std::cerr << "[Sandbox] Failed to load game module.\n";

    return 1;
  }

  std::cout << "[LevyeKit] Project: " << project->name << '\n';

  std::cout << "[LevyeKit] Assets: " << assetRoot << '\n';

  Levye::Application app(config, gameModule);

  app.Run();

  return 0;
}