#include <libGLESv2/GLES3/gl3.h>
#include <os/log.h>

#include <Levye/Core/Application.hpp>
#include <Levye/Platform/ExecutablePath.hpp>
#include <Levye/Project/ProjectLoader.hpp>
#include <memory>

extern "C" const Levye::GameAPI* GetGameAPI();

namespace {
std::unique_ptr<Levye::GameModule> game;
std::unique_ptr<Levye::Application> app;
}  // namespace

extern "C" void ios_ready() {
  if (app) return;

  const auto root = Levye::ExecutablePath::GetDirectory();
  const auto project = Levye::ProjectLoader::Load(root / "levye.project");

  if (!project) return;

  game = std::make_unique<Levye::GameModule>();
  game->SetAssetRoot((root / project->assetDirectory).string());

  if (!game->LoadStatic(GetGameAPI())) {
    game.reset();
    return;
  }

  app = std::make_unique<Levye::Application>(project->CreateApplicationConfig(), *game);

  if (!app->Initialize()) {
    os_log_error(OS_LOG_DEFAULT, "LevyeKit iOS application initialization failed");
    app.reset();
    game.reset();
  }
  if (app && glGetString(GL_VERSION) == nullptr) {
    os_log_error(OS_LOG_DEFAULT, "LevyeKit iOS GLES dispatch is unavailable");
    app->Finish();
    app.reset();
    game.reset();
  }
}

extern "C" void ios_update(bool viewSizeChanged) {
  (void)viewSizeChanged;
  if (app) app->Tick();
}

extern "C" void ios_destroy() {
  if (app) app->Finish();
  app.reset();
  game.reset();
}
