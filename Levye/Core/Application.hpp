#pragma once

#include "GameAPI.hpp"

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
 * @brief Owns the main application lifecycle and raylib window.
 *
 * Application initializes raylib, manages the main update/render loop and
 * forwards game lifecycle events through GameAPI.
 */
class Application {
public:
  /**
   * @brief Creates an application using the supplied configuration and
   * game API.
   *
   * @param config Application/window configuration.
   * @param gameAPI Function table implemented by the game module.
   */
  Application(const ApplicationConfig &config, const GameAPI &gameAPI);

  /**
   * @brief Destroys the application.
   */
  ~Application();

  /**
   * @brief Starts the application main loop.
   *
   * This function initializes the raylib window, invokes the game
   * lifecycle callbacks and runs until the window is closed.
   */
  void Run();

private:
  ApplicationConfig m_Config;

  GameAPI m_GameAPI;
  GameState m_GameState;
};
} // namespace Levye