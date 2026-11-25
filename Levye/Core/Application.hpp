#pragma once

#include <Levye/Core/ApplicationConfig.hpp>
#include <Levye/Display/DisplayManager.hpp>
#include <Levye/HotReload/GameModule.hpp>
#ifdef LEVYE_WITH_IMGUI
#include <Levye/Modules/ImGui/ImGuiService.hpp>
#endif
#include <string>

namespace Levye {

/**
 * @brief Owns the application's window and main runtime loop.
 *
 * Application keeps the host process alive while GameModule manages
 * reloadable game-specific code.
 */
class Application {
 public:
  /**
   * @brief Creates an application using the supplied configuration and
   * game API.
   *
   * @param config Application/window configuration.
   * @param gameModule Module that provides game behavior.
   */
  Application(const ApplicationConfig& config, GameModule& gameModule);

  /**
   * @brief Runs the application until the window is closed.
   */
  void Run();

  /** @brief Initializes the runtime for an externally owned frame loop. */
  bool Initialize();
  /** @brief Executes one frame; returns false when shutdown is requested. */
  bool Tick();
  /** @brief Releases the runtime while the graphics context is available. */
  void Finish();

  /**
   * @brief Requests a clean shutdown of the application.
   *
   * The application finishes its current frame and exits
   * through the normal shutdown lifecycle.
   */
  void RequestQuit();

#ifdef LEVYE_WITH_IMGUI
  [[nodiscard]] bool IsImGuiAvailable() const noexcept {
    return m_ImGui.IsInitialized();
  }
#else
  [[nodiscard]] bool IsImGuiAvailable() const noexcept {
    return false;
  }
#endif

  /** @brief Returns the host-owned display manager to host code. */
  [[nodiscard]] DisplayManager& GetDisplay() {
    return m_Display;
  }
  /** @brief Returns the host-owned display manager to read-only host code. */
  [[nodiscard]] const DisplayManager& GetDisplay() const {
    return m_Display;
  }

 private:
  void Frame();
  ApplicationConfig m_Config;
  GameModule& m_GameModule;
#ifdef LEVYE_WITH_IMGUI
  ImGuiService m_ImGui;
#endif
  DisplayManager m_Display;

  bool m_Initialized = false;
  bool m_QuitRequested = false;
};
}  // namespace Levye
