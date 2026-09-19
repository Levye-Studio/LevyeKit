#include "Application.hpp"

#include <raylib.h>

namespace Levye {
Application::Application(const ApplicationConfig &config,
                         GameModule &gameModule)
    : m_Config(config), m_GameModule(gameModule) {}

void Application::Run() {
  InitWindow(m_Config.width, m_Config.height, m_Config.title.c_str());

  SetExitKey(KEY_NULL);

  SetTargetFPS(m_Config.targetFPS);

  InitAudioDevice();

  /*
   * Game startup happens only after raylib has created the graphics context.
   * Game OnLoad callbacks may safely create GPU resources from this point.
   */
  if (!m_GameModule.Start()) {
    CloseWindow();
    return;
  }

  while (!WindowShouldClose()) {

    /*
     * Check before executing game callbacks so no function from an
     * unloaded module can be called during this frame.
     */
    m_GameModule.CheckForReload();

    m_GameModule.UpdateResources();

    const float deltaTime = GetFrameTime();

    m_GameModule.Update(deltaTime);

    BeginDrawing();

    ClearBackground(BLACK);

    m_GameModule.Draw();

    EndDrawing();
  }

  m_GameModule.Unload();
  /*
   * GPU resources must be released before raylib destroys the graphics context.
   */
  m_GameModule.ReleaseResources();

  CloseAudioDevice();

  CloseWindow();
}
} // namespace Levye