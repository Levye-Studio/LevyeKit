#include <Levye/Core/Application.hpp>
#include <Levye/HotReload/GameModule.hpp>
#include <Levye/Platform/ExecutablePath.hpp>
#include <Levye/Platform/RuntimePaths.hpp>
#include <Levye/Project/ProjectLoader.hpp>
#include <filesystem>
#include <iostream>

int main() {
  const std::filesystem::path executableDirectory = Levye::ExecutablePath::GetDirectory();

  const auto paths = Levye::RuntimePaths::Resolve(executableDirectory);
  if (!paths) {
    std::cerr << "[Levye] Invalid runtime layout.\n";
    return 1;
  }

  const auto& projectDirectory = paths->projectDirectory;
  const auto projectFile = projectDirectory / "levye.project";

  const auto project = Levye::ProjectLoader::Load(projectFile);

  if (!project) {
    std::cerr << "[Levye] Failed to load project.\n";

    return 1;
  }

  const std::filesystem::path assetRoot = projectDirectory / project->assetDirectory;

  Levye::GameModule gameModule;

  gameModule.SetAssetRoot(assetRoot.string());

  const bool loaded = paths->distribution ? gameModule.LoadDistribution(paths->modulePath.string())
                                          : gameModule.Load(paths->modulePath.string());

  if (!loaded) {
    return 1;
  }

  const Levye::ApplicationConfig applicationConfig = project->CreateApplicationConfig();

  Levye::Application application{applicationConfig, gameModule};

  application.Run();

  return 0;
}