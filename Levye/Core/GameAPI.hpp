#pragma once
#include "HostServices.hpp"
#include "Services.hpp"
#include <Levye/Assets/AssetHandle.hpp>
#include <raylib.h>

#include <cstddef>
#include <cstdint>

/** @brief Exports the unmangled entry point of a reloadable game module. */
#if defined(_WIN32)
#define LEVYE_GAME_EXPORT extern "C" __declspec(dllexport)
#else
#define LEVYE_GAME_EXPORT extern "C"
#endif

namespace Levye {
/**
 * @brief Version of the binary interface shared by the host and game module.
 */
inline constexpr std::uint32_t GAME_API_VERSION = 13;

/**
 * @brief Function table exposed by every Levye game module.
 */
struct GameAPI {
  std::uint32_t version = GAME_API_VERSION;

  /**
   * @brief Binds host services to the currently loaded game module.
   *
   * This callback is invoked immediately after a module is loaded and
   * before any other game callback executes.
   *
   * @param services Host service table provided by LevyeKit.
   */
  void (*BindServices)(const HostServices *services) = nullptr;

  /// Called after the game module has been loaded.
  void (*OnLoad)(void *state) = nullptr;

  /// Called after game code has been successfully hot reloaded.
  void (*OnBeforeReload)(void *) = nullptr;

  void (*OnAfterReload)(void *) = nullptr;

  /// Called once per frame before rendering.
  void (*OnUpdate)(void *state, float deltaTime) = nullptr;

  /**
   * @brief Runs one fixed-rate simulation step.
   *
   * Fixed updates are intended for deterministic simulation such as movement,
   * collision detection, and future physics systems.
   */
  void (*OnFixedUpdate)(void *state, float fixedDeltaTime) = nullptr;

  /// Called once per frame while a raylib drawing context is active.
  void (*OnDraw)(void *state) = nullptr;

  /// Called before the game module is unloaded.
  void (*OnShutdown)(void *state) = nullptr;

  /**
   * @brief Initializes newly allocated persistent game state.
   *
   * Called once after the host creates the state storage.
   *
   * @param state Host-owned persistent state memory.
   * @param services Services provided by the application host.
   */
  void (*InitializeState)(void *state) = nullptr;

  /**
   * @brief Performs final cleanup of persistent game state.
   *
   * Called during application shutdown before the state storage is released.
   *
   * @param state Persistent game state.
   * @param services Services provided by the application host.
   */
  void (*DestroyState)(void *state) = nullptr;

  /**
   * @brief Size in bytes required for persistent game state.
   */
  std::size_t stateSize = 0;
};

/**
 * @brief Signature of the entry point exported by a Levye game module.
 */
using GetGameAPIFn = GameAPI *(*)();
} // namespace Levye
