#pragma once

#include <Levye/Core/ApplicationConfig.hpp>

#include <string>

namespace Levye {

/**
 * @brief Describes a LevyeKit game project.
 *
 * ProjectConfig contains project-level information used by the framework to
 * locate game assets and configure the application at startup.
 *
 * Unlike ApplicationConfig, which describes the running window and renderer,
 * ProjectConfig describes the game project itself.
 */
struct ProjectConfig {
  /**
   * @brief Human-readable project name.
   */
  std::string name = "Levye Game";

  /**
   * @brief Root directory containing project assets.
   *
   * Relative asset paths requested through HostServices are resolved from
   * this directory.
   */
  std::string assetDirectory = "Assets";

  /**
   * @brief Initial width of the game window in pixels.
   */
  int windowWidth = 1280;

  /**
   * @brief Initial height of the game window in pixels.
   */
  int windowHeight = 720;

  /**
   * @brief Target rendering frame rate.
   *
   * A value less than or equal to zero leaves the frame rate unrestricted.
   */
  int targetFPS = 60;

  /**
   * @brief Whether the game window can be resized by the user.
   */
  bool resizable = true;

  /**
   * @brief Whether vertical synchronization should be requested.
   */
  bool vsync = false;

  /**
   * @brief Creates the runtime application configuration for this project.
   *
   * @return Application configuration containing the project's window and
   *         rendering settings.
   */
  ApplicationConfig CreateApplicationConfig() const;
};

} // namespace Levye