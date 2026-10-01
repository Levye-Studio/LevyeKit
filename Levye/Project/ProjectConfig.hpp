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
   * @brief Human-readable name displayed to the player.
   *
   * This value may contain spaces and is used for things such as the
   * application window title.
   */
  std::string name = "Levye Game";

  /**
   * @brief Build-safe identifier used for the executable and CMake target.
   *
   * The target should not contain spaces or characters that are unsuitable
   * for build-system identifiers.
   */
  std::string target = "LevyeGame";

  /**
   * @brief Root directory containing project assets.
   */
  std::string assetDirectory = "Assets";

  int windowWidth = 1280;
  int windowHeight = 720;
  int targetFPS = 60;
  bool resizable = true;
  bool vsync = false;

  /**
   * @brief Creates the runtime application configuration for this project.
   *
   * @return Application configuration derived from the project settings.
   */
  ApplicationConfig CreateApplicationConfig() const;
};

} // namespace Levye