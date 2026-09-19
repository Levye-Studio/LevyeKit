#include "Application.hpp"

#include <raylib.h>

namespace Levye {
Application::Application(const ApplicationConfig &config,
                         GameModule &gameModule)
    : m_Config(config), m_GameModule(gameModule) {}

void Application::Run() {
  InitWindow(m_Config.width, m_Config.height, m_Config.title.c_str());

  SetTargetFPS(m_Config.targetFPS);
  SetExitKey(NULL);

  while (!WindowShouldClose()) {

    /*
     * Check before executing game callbacks so no function from an
     * unloaded module can be called during this frame.
     */
    m_GameModule.CheckForReload();

    const float deltaTime = GetFrameTime();

    m_GameModule.Update(deltaTime);

    BeginDrawing();

    ClearBackground(BLACK);

    m_GameModule.Draw();

    EndDrawing();
  }

  CloseWindow();
}
} // namespace Levye