#pragma once
#include <raylib.h>

#include <Levye/Assets/AssetHandle.hpp>
#include <cstddef>
#include <cstdint>

#include "HostServices.hpp"
#include "Services.hpp"

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
inline constexpr std::uint32_t GAME_API_VERSION = 17;

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
  void (*BindServices)(const HostServices* services) = nullptr;

  /** @brief Runs after the game module has been loaded. */
  void (*OnLoad)(void* state) = nullptr;

  /** @brief Runs before the current game code is unloaded for hot reload. */
  void (*OnBeforeReload)(void*) = nullptr;

  /** @brief Runs after replacement game code has been loaded successfully. */
  void (*OnAfterReload)(void*) = nullptr;

  /** @brief Updates gameplay once per rendered frame. */
  void (*OnUpdate)(void* state, float deltaTime) = nullptr;

  /**
   * @brief Runs one fixed-rate simulation step.
   *
   * Fixed updates are intended for deterministic simulation such as movement,
   * collision detection, and future physics systems.
   */
  void (*OnFixedUpdate)(void* state, float fixedDeltaTime) = nullptr;

  /** @brief Draws the game once per frame inside an active raylib drawing context. */
  void (*OnDraw)(void* state) = nullptr;

  /** @brief Runs when the game module shuts down. */
  void (*OnShutdown)(void* state) = nullptr;

  /**
   * @brief Initializes newly allocated persistent game state.
   *
   * Called once after the host creates the state storage.
   *
   * @param state Host-owned persistent state memory.
   * @param services Services provided by the application host.
   */
  void (*InitializeState)(void* state) = nullptr;

  /**
   * @brief Performs final cleanup of persistent game state.
   *
   * Called during application shutdown before the state storage is released.
   *
   * @param state Persistent game state.
   * @param services Services provided by the application host.
   */
  void (*DestroyState)(void* state) = nullptr;

  /**
   * @brief Size in bytes required for persistent game state.
   */
  std::size_t stateSize = 0;
};

/**
 * @brief Signature of the entry point exported by a Levye game module.
 */
using GetGameAPIFn = GameAPI* (*)();
}  // namespace Levye
