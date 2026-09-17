#include <Levye/Core/Application.hpp>
#include <Levye/HotReload/GameModule.hpp>

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

  if (!gameModule.Load(gameLibraryPath)) {
    std::cerr << "[Sandbox] Failed to load game module.\n";

    return 1;
  }

  const Levye::GameAPI *gameAPI = gameModule.GetAPI();

  if (!gameAPI)
    return 1;

  Levye::ApplicationConfig config;

  config.width = 1280;
  config.height = 720;
  config.title = "LevyeKit Sandbox";
  config.targetFPS = 60;

  Levye::Application app(config, *gameAPI);

  app.Run();

  return 0;
}