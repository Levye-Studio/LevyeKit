#include "Application.hpp"

#include <raylib.h>

namespace Levye {
Application::Application(const ApplicationConfig &config,
                         GameModule &gameModule)
    : m_Config(config), m_GameModule(gameModule) {}

void Application::Run() {
  unsigned int windowFlags = 0;

  if (m_Config.resizable) {
    windowFlags |= FLAG_WINDOW_RESIZABLE;
  }

  if (m_Config.vsync) {
    windowFlags |= FLAG_VSYNC_HINT;
  }

  if (windowFlags != 0) {
    SetConfigFlags(windowFlags);
  }

  InitWindow(m_Config.width, m_Config.height, m_Config.title.c_str());

  SetExitKey(KEY_NULL);

  if (m_Config.targetFPS > 0) {
    SetTargetFPS(m_Config.targetFPS);
  }

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

    m_GameModule.UpdateHostSystems();

    Time &time = m_GameModule.GetTime();

    /*
     * Capture the real frame duration once. Every timing system for this frame
     * derives from this value.
     */
    time.Update(GetFrameTime());

    /*
     * Frame-rate-dependent gameplay and input processing run once per rendered
     * frame.
     */
    m_GameModule.Update(time.GetDeltaTime());

    /*
     * Simulation may run zero, one, or several times depending on how much
     * scaled game time accumulated since the previous rendered frame.
     */
    const int fixedSteps = time.ConsumeFixedSteps();

    for (int i = 0; i < fixedSteps; ++i) {
      m_GameModule.FixedUpdate(time.GetFixedDeltaTime());
    }

    BeginDrawing();

    ClearBackground(BLACK);

    m_GameModule.Draw();

    EndDrawing();
  }

  m_GameModule.Shutdown();
  /*
   * GPU resources must be released before raylib destroys the graphics context.
   */
  m_GameModule.ReleaseResources();

  CloseAudioDevice();

  CloseWindow();
}
} // namespace Levye