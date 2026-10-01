#include <raylib.h>

#include <Levye/Core/Application.hpp>
#include <Levye/HotReload/GameModule.hpp>
#include <Levye/Platform/ExecutablePath.hpp>
#include <Levye/Platform/SharedLibrary.hpp>
#include <Levye/Project/ProjectConfig.hpp>
#include <Levye/Project/ProjectLoader.hpp>
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