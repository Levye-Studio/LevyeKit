#pragma once

#include <Levye/Core/ApplicationConfig.hpp>
#include <Levye/HotReload/GameModule.hpp>

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