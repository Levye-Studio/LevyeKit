#pragma once

namespace Levye {

/**
 * @brief Controls whether Dear ImGui blocks gameplay input.
 *
 * These settings affect LevyeKit's mapped input system,
 * not Dear ImGui's ability to receive input.
 */
struct ImGuiCaptureSettings {
  bool keyboard = true;
  bool mouse = true;
};

/**
 * @brief Host-owned Dear ImGui lifecycle service.
 *
 * The service owns initialization and shutdown of the Dear ImGui context and
 * its raylib backend. It remains alive independently of reloadable game code.
 */
class ImGuiService {
 public:
  /**
   * @brief Initializes Dear ImGui and its raylib integration.
   *
   * @return true when initialization succeeds.
   */
  bool Initialize();

  /**
   * @brief Starts a new ImGui frame.
   *
   * Called once per host frame before game drawing.
   */
  void BeginFrame();

  /**
   * @brief Finishes and renders the current ImGui frame.
   */
  void EndFrame();

  /**
   * @brief Shuts down the ImGui backend and destroys its context.
   */
  void Shutdown();

  /**
   * @brief Returns whether the service is currently initialized.
   */
  [[nodiscard]] bool IsInitialized() const { return m_Initialized; }

  /**
   * @brief Updates gameplay input capture preferences.
   *
   * @param settings Capture preferences for keyboard and mouse.
   */
  void SetCaptureSettings(const ImGuiCaptureSettings& settings);

  /**
   * @brief Returns the current capture preferences.
   */
  [[nodiscard]] ImGuiCaptureSettings GetCaptureSettings() const;

 private:
  bool m_Initialized = false;
  ImGuiCaptureSettings m_CaptureSettings{};
};

}  // namespace Levye