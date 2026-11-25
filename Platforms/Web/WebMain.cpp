#include <Levye/Core/Application.hpp>
#include <Levye/Project/ProjectLoader.hpp>
#include <iostream>
#include <memory>

extern "C" const Levye::GameAPI* GetGameAPI();

int main() {
  const auto project = Levye::ProjectLoader::Load("/levye.project");
  if (!project)
    return 1;
  // These objects must outlive main: browser callbacks run asynchronously.
  static Levye::GameModule game;
  game.SetAssetRoot("/" + project->assetDirectory);
  if (!game.LoadStatic(GetGameAPI()))
    return 1;
  static auto app = std::make_unique<Levye::Application>(project->CreateApplicationConfig(), game);
  app->Run();
  return 0;
}
