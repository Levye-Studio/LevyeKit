#include "Application.hpp"

#include <raylib.h>
#ifdef LEVYE_WITH_IMGUI
#include <imgui.h>
#endif

#include <Levye/Debug/Logger.hpp>

namespace Levye {
Application::Application(const ApplicationConfig& config,
                         GameModule& gameModule)
    : m_Config(config), m_GameModule(gameModule) {
  m_GameModule.SetApplication(this);
}

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

#ifdef LEVYE_WITH_IMGUI
  if (!m_ImGui.Initialize()) {
    Logger::Error("Failed to initialize ImGui module.");
    CloseWindow();
    return;
  }

  m_ImGui.SetCaptureSettings({.keyboard = true, .mouse = true});
#endif

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
#ifdef LEVYE_WITH_IMGUI
    m_ImGui.Shutdown();
#endif

    CloseAudioDevice();
    CloseWindow();
    return;
  }

  while (!WindowShouldClose() && !m_QuitRequested) {
#ifdef LEVYE_WITH_IMGUI
    const ImGuiIO& io = ImGui::GetIO();

    const ImGuiCaptureSettings settings = m_ImGui.GetCaptureSettings();

    m_GameModule.GetInputMap().SetInputCapture(
        settings.keyboard && io.WantCaptureKeyboard,
        settings.mouse && io.WantCaptureMouse);
#endif

    // raylib polls events at EndDrawing(). Sample once before any callbacks,
    // including reload callbacks, so every callback sees this frame's state.
    m_GameModule.GetInputMap().Update();

    /*
     * Check before executing game callbacks so no function from an
     * unloaded module can be called during this frame.
     */
    m_GameModule.CheckForReload();

    m_GameModule.UpdateHostSystems();

    TimeSystem& time = m_GameModule.GetTimeSystem();

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

#ifdef LEVYE_WITH_IMGUI
    m_ImGui.BeginFrame();
#endif

    m_GameModule.Draw();

#ifdef LEVYE_WITH_IMGUI
    m_ImGui.EndFrame();
#endif

    EndDrawing();
  }

  m_GameModule.Shutdown();
  /*
   * GPU resources must be released before raylib destroys the graphics context.
   */
  m_GameModule.ReleaseResources();

#ifdef LEVYE_WITH_IMGUI
  m_ImGui.Shutdown();
#endif

  CloseAudioDevice();

  CloseWindow();
}

void Application::RequestQuit() { m_QuitRequested = true; }
}  // namespace Levye