#include "Application.hpp"

#include <raylib.h>
#if defined(PLATFORM_IOS)
#include <libGLESv2/GLES3/gl3.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif
#ifdef LEVYE_WITH_IMGUI
#include <imgui.h>
#endif

#include <Levye/Debug/Logger.hpp>

namespace Levye {
#if defined(PLATFORM_IOS)
extern "C" void LevyeIOSSafeInsets(int framebufferWidth, int framebufferHeight,
                                   DisplayInsets* outputInsets);
#elif defined(__ANDROID__)
extern "C" void LevyeAndroidSafeInsets(DisplayInsets* outputInsets);
extern "C" void LevyeAndroidEnableCutout();
#endif
#ifdef __EMSCRIPTEN__
namespace {
void GetBrowserSize(int& width, int& height) {
  double cssWidth = 0.0;
  double cssHeight = 0.0;
  if (emscripten_get_element_css_size("#canvas", &cssWidth, &cssHeight) ==
          EMSCRIPTEN_RESULT_SUCCESS &&
      cssWidth >= 1.0 && cssHeight >= 1.0) {
    width = static_cast<int>(cssWidth);
    height = static_cast<int>(cssHeight);
  }
}
}  // namespace
#endif

Application::Application(const ApplicationConfig& config, GameModule& gameModule)
    : m_Config(config), m_GameModule(gameModule) {
  m_GameModule.SetApplication(this);
}

bool Application::Initialize() {
  if (m_Initialized)
    return true;
  unsigned int windowFlags = 0;

  if (m_Config.resizable) {
    windowFlags |= FLAG_WINDOW_RESIZABLE;
  }

  if (m_Config.vsync) {
    windowFlags |= FLAG_VSYNC_HINT;
  }

#if !defined(__ANDROID__) && !defined(PLATFORM_IOS) && !defined(__EMSCRIPTEN__)
  if (m_Config.fullscreen)
    windowFlags |= FLAG_FULLSCREEN_MODE;
#endif

#if !defined(__EMSCRIPTEN__) && !defined(PLATFORM_IOS)
  if (m_Config.useLogicalDisplay) {
    windowFlags |= FLAG_WINDOW_HIGHDPI;
  }
#elif defined(__EMSCRIPTEN__)
  windowFlags |= FLAG_WINDOW_RESIZABLE;
#else
  windowFlags |= FLAG_FULLSCREEN_MODE;
#endif

  if (windowFlags != 0) {
    SetConfigFlags(windowFlags);
  }

#if defined(__ANDROID__) || defined(PLATFORM_IOS)
#if defined(__ANDROID__)
  if (m_Config.useLogicalDisplay)
    LevyeAndroidEnableCutout();
#endif
  // raylib sizes its Android framebuffer from InitWindow's dimensions and
  // applies its own scale matrix when they differ from the native surface.
  // The manifest's [desktop] size is for desktop; logical scaling is handled
  // by DisplayManager after raylib adopts the Android surface size.
  InitWindow(0, 0, m_Config.title.c_str());
#elif defined(__EMSCRIPTEN__)
  int width = m_Config.width;
  int height = m_Config.height;
  GetBrowserSize(width, height);
  InitWindow(width, height, m_Config.title.c_str());
#else
  InitWindow(m_Config.width, m_Config.height, m_Config.title.c_str());
#endif

  if (!IsWindowReady())
    return false;

#if defined(PLATFORM_IOS)
  // Validate GLES dispatch before creating the logical display's GPU resources.
  if (glGetString(GL_VERSION) == nullptr) {
    Logger::Error("iOS OpenGL ES context has no active GL version.");
    CloseWindow();
    return false;
  }
#endif

#ifdef LEVYE_WITH_IMGUI
  if (!m_ImGui.Initialize()) {
    Logger::Error("Failed to initialize ImGui module.");
    CloseWindow();
    return false;
  }

  m_ImGui.SetCaptureSettings({.keyboard = true, .mouse = true});
#endif

  if (m_Config.useLogicalDisplay) {
    if (!m_Display.Initialize(m_Config.display)) {
      Logger::Error("Failed to initialize logical display.");
#ifdef LEVYE_WITH_IMGUI
      m_ImGui.Shutdown();
#endif
      CloseWindow();
      return false;
    }
  }

  SetExitKey(KEY_NULL);

  // Browser requestAnimationFrame owns frame pacing.
#if defined(__EMSCRIPTEN__) || defined(PLATFORM_IOS)
  SetTargetFPS(0);
#else
  if (m_Config.targetFPS > 0) {
    SetTargetFPS(m_Config.targetFPS);
  }
#endif

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
    m_Display.Shutdown();
    CloseWindow();
    return false;
  }

  m_Initialized = true;
  return true;
}

void Application::Run() {
  if (!Initialize())
    return;
#if defined(PLATFORM_IOS)
  // UIKit owns the frame loop.
  return;
#elif defined(__EMSCRIPTEN__)
  emscripten_set_main_loop_arg(
      [](void* context) {
        auto& app = *static_cast<Application*>(context);
        if (app.m_QuitRequested || WindowShouldClose()) {
          emscripten_cancel_main_loop();
          app.Finish();
          return;
        }
        app.Frame();
      },
      this, 0, false);
#else
  while (!WindowShouldClose() && !m_QuitRequested)
    Frame();
  Finish();
#endif
}

bool Application::Tick() {
  if (!m_Initialized || m_QuitRequested || WindowShouldClose())
    return false;
  Frame();
  return true;
}

void Application::Frame() {
#ifdef __EMSCRIPTEN__
  // Keep raylib window coordinates aligned with browser CSS pixels.
  int width = GetScreenWidth();
  int height = GetScreenHeight();
  GetBrowserSize(width, height);
  if (width != GetScreenWidth() || height != GetScreenHeight()) {
    SetWindowSize(width, height);
  }
#endif
#if defined(__ANDROID__)
  static double lastAndroidFrame = 0.0;
  static bool lastAndroidFocus = true;
  const double androidFrameTime = GetTime();
  const bool androidFocus = IsWindowFocused();
  if (androidFocus != lastAndroidFocus ||
      (lastAndroidFrame > 0.0 && androidFrameTime - lastAndroidFrame > 1.0)) {
    TraceLog(LOG_WARNING, "ANDROID LIFECYCLE: focus=%d previousFocus=%d frameGap=%.3f",
             androidFocus, lastAndroidFocus, androidFrameTime - lastAndroidFrame);
  }
  lastAndroidFrame = androidFrameTime;
  lastAndroidFocus = androidFocus;
#endif
#ifdef LEVYE_WITH_IMGUI
  const ImGuiIO& io = ImGui::GetIO();

  const ImGuiCaptureSettings settings = m_ImGui.GetCaptureSettings();

  m_GameModule.GetInputMap().SetInputCapture(settings.keyboard && io.WantCaptureKeyboard,
                                             settings.mouse && io.WantCaptureMouse);
#endif

  if (m_Config.useLogicalDisplay) {
    m_Display.Resize(GetScreenWidth(), GetScreenHeight(), GetRenderWidth(), GetRenderHeight());
#if defined(PLATFORM_IOS)
    DisplayInsets nativeInsets{};
    LevyeIOSSafeInsets(GetRenderWidth(), GetRenderHeight(), &nativeInsets);
    m_Display.SetSafeInsets(nativeInsets);
#elif defined(__ANDROID__)
    DisplayInsets nativeInsets{};
    LevyeAndroidSafeInsets(&nativeInsets);
    m_Display.SetSafeInsets(nativeInsets);
#endif
  }

  // raylib polls events at EndDrawing(). Sample once before any callbacks,
  // including reload callbacks, so every callback sees this frame's state.
  m_GameModule.GetInputMap().Update();
  m_GameModule.SampleTouchInput();
#if defined(__ANDROID__)
  static unsigned int touchDiagnosticFrame = 0;
  if ((++touchDiagnosticFrame % 120) == 0 || GetTouchPointCount() > 0) {
    TraceLog(LOG_WARNING,
             "ANDROID FRAME: frame=%u focused=%d nativeTouches=%d screen=%dx%d render=%dx%d",
             touchDiagnosticFrame, IsWindowFocused(), GetTouchPointCount(), GetScreenWidth(),
             GetScreenHeight(), GetRenderWidth(), GetRenderHeight());
  }
#endif

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

  if (m_Config.useLogicalDisplay) {
    m_Display.BeginGame();
  }

  m_GameModule.Draw();

  if (m_Config.useLogicalDisplay) {
    m_Display.EndGame();
    m_Display.Present();
  }

#ifdef LEVYE_WITH_IMGUI
  m_ImGui.EndFrame();
#endif

  EndDrawing();
}

void Application::Finish() {
  if (!m_Initialized)
    return;
  m_Initialized = false;
  m_GameModule.Shutdown();
  /*
   * GPU resources must be released before raylib destroys the graphics context.
   */
  m_GameModule.ReleaseResources();

#ifdef LEVYE_WITH_IMGUI
  m_ImGui.Shutdown();
#endif

  CloseAudioDevice();
  m_Display.Shutdown();

  CloseWindow();
}

void Application::RequestQuit() {
  m_QuitRequested = true;
}
}  // namespace Levye
