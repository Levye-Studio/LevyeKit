#include <Levye/Core/Application.hpp>
#include <Levye/Core/GameAPI.hpp>
#include <Levye/HotReload/GameModule.hpp>

extern "C" const Levye::GameAPI* GetGameAPI();

#ifndef LEVYE_ANDROID_TITLE
#define LEVYE_ANDROID_TITLE "LevyeKit Sandbox"
#endif
#ifndef LEVYE_ANDROID_ASSET_ROOT
#define LEVYE_ANDROID_ASSET_ROOT "Assets"
#endif
#ifndef LEVYE_ANDROID_LOGICAL_WIDTH
#define LEVYE_ANDROID_LOGICAL_WIDTH 720
#endif
#ifndef LEVYE_ANDROID_LOGICAL_HEIGHT
#define LEVYE_ANDROID_LOGICAL_HEIGHT 1280
#endif

int main() {
  Levye::GameModule game;
  game.SetAssetRoot(LEVYE_ANDROID_ASSET_ROOT);
  game.GetScreenManager().SetScreen("Menu");
  if (!game.LoadStatic(GetGameAPI())) return 1;

  Levye::ApplicationConfig config;
  config.title = LEVYE_ANDROID_TITLE;
  config.width = LEVYE_ANDROID_LOGICAL_WIDTH;
  config.height = LEVYE_ANDROID_LOGICAL_HEIGHT;
  config.targetFPS = 60;
  config.useLogicalDisplay = true;
  config.display.logical = {LEVYE_ANDROID_LOGICAL_WIDTH, LEVYE_ANDROID_LOGICAL_HEIGHT};
  config.display.scaling = Levye::DisplayScaling::Fit;

  Levye::Application app(config, game);
  app.Run();
  return 0;
}
