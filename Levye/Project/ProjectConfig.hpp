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

  /** @brief Default platform for `levye run`; desktop when omitted. */
  std::string defaultPlatform = "desktop";

  /** @brief Optional iOS application metadata. */
  std::string iosBundleId;
  std::string iosDeploymentTarget = "15.0";
  std::string iosDisplayName;
  std::string iosVersionName = "1.0.0";
  int iosVersionCode = 1;
  /** @brief Legacy [ios] orientation fallback; new projects use displayOrientation. */
  std::string iosOrientation = "all";

  /** @brief Android application ID used by `levye run --android`. */
  std::string androidPackage;

  /** @brief Legacy [android] orientation fallback; new projects use [display]. */
  std::string androidOrientation = "portrait";
  /** @brief Shared mobile orientation; auto follows device rotation where supported. */
  std::string displayOrientation = "landscape";
  /** @brief Whether [display].orientation was explicitly configured. */
  bool hasDisplayOrientation = false;

  /** @brief Optional Android launcher label; defaults to the project target. */
  std::string androidAppName;
  /** @brief Android package version metadata. */
  std::string androidVersionName = "1.0";
  int androidVersionCode = 1;
  /** @brief Launcher images relative to the project asset directory. */
  std::string androidIcon;
  std::string androidAdaptiveIconForeground;
  /** @brief Image path relative to assets, or a #RRGGBB/#AARRGGBB color. */
  std::string androidAdaptiveIconBackground = "#202A44";

  /** @brief Optional production signing key location and alias. Passwords are never stored here. */
  std::string androidSigningKeystore;
  std::string androidSigningAlias;

  /** @brief Optional desktop distribution identity. */
  std::string desktopAppName;
  std::string desktopBundleId;
  std::string desktopVersionName = "1.0.0";
  int desktopVersionCode = 1;
  std::string desktopIcon;

  int windowWidth = 1280;
  int windowHeight = 720;
  int targetFPS = 60;
  bool resizable = true;
  bool vsync = false;
  bool fullscreen = false;

  /** @brief Enables the host-owned logical viewport when [display] is configured. */
  bool useLogicalDisplay = false;
  /** @brief Logical game size and scaling; independent of the initial window size. */
  DisplayConfig display{};

  /**
   * @brief Creates the runtime application configuration for this project.
   *
   * @return Application configuration derived from the project settings.
   */
  ApplicationConfig CreateApplicationConfig() const;
};

}  // namespace Levye
