#pragma once

#include <string>

namespace Levye {
/**
 * @brief Configuration used when creating a LevyeKit application.
 *
 * ApplicationConfig contains startup settings that normally remain
 * constant for the lifetime of the application.
 */
struct ApplicationConfig {
  /**
   * @brief Title displayed by the application window.
   */
  std::string title = "LevyeKit";

  /**
   * @brief Initial width of the application window in pixels.
   */
  int width = 1280;

  /**
   * @brief Initial height of the application window in pixels.
   */
  int height = 720;

  /**
   * @brief Target rendering frame rate.
   *
   * A value greater than zero causes LevyeKit to call SetTargetFPS().
   */
  int targetFPS = 60;

  /**
   * @brief Whether the window should be resizable.
   */
  bool resizable = true;

  /**
   * @brief Whether vertical synchronization should be requested.
   */
  bool vsync = false;
};
} // namespace Levye