#pragma once

#include <Levye/HotReload/GameModule.hpp>

#include <string>

namespace Levye {
/**
 * @brief Configuration used when creating a Levye application.
 */
struct ApplicationConfig {
  /// Initial width of the application window.
  int width = 1280;

  /// Initial height of the application window.
  int height = 720;

  /// Title displayed by the application window.
  std::string title = "Levye Game";

  /// Target number of frames rendered per second.
  int targetFPS = 60;
};

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
  Application(const ApplicationConfig &config, GameModule &gameModule);

  /**
   * @brief Runs the application until the window is closed.
   */
  void Run();

private:
  ApplicationConfig m_Config;
  GameModule &m_GameModule;
};
} // namespace Levye