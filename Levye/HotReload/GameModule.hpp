#pragma once

#include <Levye/Assets/TextureManager.hpp>
#include <Levye/Core/GameAPI.hpp>
#include <Levye/Core/HostContext.hpp>
#include <Levye/Core/HostServices.hpp>
#include <Levye/HotReload/DynamicLibrary.hpp>
#include <Levye/Input/InputMap.hpp>
#include <Levye/Screen/ScreenManager.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>

namespace Levye {
/**
 * @brief Owns and manages a dynamically reloadable Levye game module.
 *
 * The build output is never loaded directly. GameModule creates a runtime
 * copy and loads that copy instead. This prevents the build system from
 * overwriting a library that is currently executing.
 *
 * Persistent GameState remains owned by this object while the executable
 * code contained in the dynamic library can be replaced.
 */
class GameModule {
public:
  GameModule() = default;

  /**
   * @brief Unloads the module and removes temporary runtime copies.
   */
  ~GameModule();

  GameModule(const GameModule &) = delete;
  GameModule &operator=(const GameModule &) = delete;

  /**
   * @brief Loads the initial game module.
   *
   * @param path Path to the build output shared library.
   * @return true if a valid runtime copy was loaded.
   */
  bool Load(const std::string &path);

  /**
   * @brief Starts the loaded game module.
   *
   * Start must be called after the host has initialized the systems required by
   * game code, including raylib's graphics context.
   *
   * @return true when the game was started successfully.
   */
  bool Start();

  /**
   * @brief Checks the build output for changes and attempts a hot reload.
   *
   * A new runtime copy is validated before the currently running module
   * is replaced.
   *
   * @return true when a new module was installed successfully.
   */
  bool CheckForReload();

  /**
   * @brief Unloads the currently active game module.
   */
  void Unload();

  /**
   * @brief Calls the current module's update callback.
   *
   * @param deltaTime Time elapsed since the previous frame.
   */
  void Update(float deltaTime);

  /**
   * @brief Updates host-owned game resources.
   *
   * This currently checks loaded textures for external file changes.
   */
  void UpdateResources();

  /**
   * @brief Calls the active game's drawing callback.
   */
  void Draw();

  /**
   * @brief Returns whether a valid game module is currently active.
   */
  bool IsLoaded() const;

  /**
   * @brief Returns the original build-output path being watched.
   */
  const std::string &GetPath() const;

  /**
   * @brief Returns the host-owned input map.
   *
   * This allows the application or project configuration to register logical
   * game actions while keeping the map alive across game-code reloads.
   */
  InputMap &GetInputMap();

  /**
   * @brief Returns the host-owned screen manager.
   *
   * The active screen survives game-code hot reloads because the manager is
   * stored outside the reloadable game module.
   */
  ScreenManager &GetScreenManager();

  /**
   * @brief Releases host-owned game resources.
   *
   * This must be called while the raylib graphics and audio contexts are still
   * active.
   */
  void ReleaseResources();

private:
  /**
   * @brief Creates a unique path for the next runtime module copy.
   */
  std::filesystem::path CreateRuntimePath();

  /**
   * @brief Copies the build output into the hot-reload directory.
   *
   * @param destination Destination runtime path.
   * @return true if the copy completed successfully.
   */
  bool CopyModule(const std::filesystem::path &destination);

  /**
   * @brief Loads and validates a module without changing the active one.
   *
   * @param path Runtime library to test.
   * @param library Receives ownership of the loaded library.
   * @param api Receives the module's GameAPI.
   * @return true if the module and API are valid.
   */
  bool LoadCandidate(const std::filesystem::path &path, DynamicLibrary &library,
                     GameAPI &api);

  /**
   * @brief Removes runtime module copies that are no longer needed.
   */
  void CleanupRuntimeFiles();

private:
  DynamicLibrary m_Library;

  GameAPI m_API{};
  GameState m_State{};
  HostServices m_HostServices{};
  HostContext m_HostContext{};

  InputMap m_InputMap;
  TextureManager m_TextureManager;
  ScreenManager m_ScreenManager;

  std::string m_Path;

  std::filesystem::path m_RuntimePath;
  std::filesystem::path m_RuntimeDirectory;

  std::filesystem::file_time_type m_LastWriteTime{};

  std::uint64_t m_RuntimeGeneration = 0;

  bool m_HasAPI = false;
  bool m_Started = false;

  std::filesystem::file_time_type m_PendingWriteTime{};

  std::chrono::steady_clock::time_point m_ChangeDetectedAt{};

  bool m_ReloadPending = false;

  /**
   * Time the build output must remain unchanged before LevyeKit attempts
   * to copy and load it.
   */
  static constexpr std::chrono::milliseconds ReloadDebounce{200};
};
} // namespace Levye