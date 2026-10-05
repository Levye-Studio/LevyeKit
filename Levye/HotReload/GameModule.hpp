#pragma once

#include <Levye/Assets/TextureManager.hpp>
#include <Levye/Audio/AudioManager.hpp>
#include <Levye/Core/GameAPI.hpp>
#include <Levye/Core/HostContext.hpp>
#include <Levye/Core/HostServices.hpp>
#include <Levye/Graphics/FontManager.hpp>
#include <Levye/Graphics/ShaderManager.hpp>
#include <Levye/HotReload/DynamicLibrary.hpp>
#include <Levye/IO/FileWatcher.hpp>
#include <Levye/Input/InputMap.hpp>
#include <Levye/Screen/ScreenManager.hpp>
#include <Levye/Time/TimeSystem.hpp>
#ifdef LEVYE_WITH_SERIALIZATION
#include <Levye/Modules/Serialization/SerializationBridge.hpp>
#include <Levye/Modules/Serialization/SerializationService.hpp>
#endif

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>

namespace Levye {
class Application;
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

  GameModule(const GameModule&) = delete;
  GameModule& operator=(const GameModule&) = delete;

  /**
   * @brief Loads the initial game module.
   *
   * @param path Path to the build output shared library.
   * @return true if a valid runtime copy was loaded.
   */
  bool Load(const std::string& path);

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
   * @brief Shuts down the active game and unloads its dynamic module.
   *
   * OnShutdown is invoked before the module is unloaded when the game has
   * previously been started.
   */
  void Shutdown();

  /**
   * @brief Calls the current module's update callback.
   *
   * @param deltaTime Time elapsed since the previous frame.
   */
  void Update(float deltaTime);

  /**
   * @brief Advances the host-owned time system.
   *
   * @param deltaTime Real frame duration in seconds.
   */
  void UpdateTime(float deltaTime);

  /**
   * @brief Updates persistent host-owned systems.
   *
   * Processes asset hot reloads and updates systems that require per-frame host
   * maintenance, such as streaming audio.
   */
  void UpdateHostSystems();

  /**
   * @brief Executes one fixed simulation update.
   *
   * @param fixedDeltaTime Duration of the simulation step in seconds.
   */
  void FixedUpdate(float fixedDeltaTime);

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
  const std::string& GetPath() const;

  /**
   * @brief Returns the host-owned time system.
   */
  TimeSystem& GetTimeSystem();

  /**
   * @brief Returns the host-owned input map.
   *
   * This allows the application or project configuration to register logical
   * game actions while keeping the map alive across game-code reloads.
   */
  InputMap& GetInputMap();

  /**
   * @brief Returns the host-owned screen manager.
   *
   * The active screen survives game-code hot reloads because the manager is
   * stored outside the reloadable game module.
   */
  ScreenManager& GetScreenManager();

  /**
   * @brief Releases host-owned game resources.
   *
   * This must be called while the raylib graphics and audio contexts are still
   * active.
   */
  void ReleaseResources();

  /**
   * @brief Sets the root directory used to resolve relative asset paths.
   *
   * The asset root should normally be configured before Start() so game code
   * can load relative assets during OnLoad().
   *
   * @param path Project asset directory.
   */
  void SetAssetRoot(const std::string& path);

  /**
   * @brief Returns the configured project asset root.
   */
  const std::string& GetAssetRoot() const;

  void SetApplication(Application* application);

 private:
  /**
   * @brief Creates a unique path for the next runtime module copy.
   */
  std::filesystem::path CreateRuntimePath();

  /**
   * @brief Copies the build output into the hot-reload directory.
   *
   * @param destination Destination runtime path.
   * @return Success, a retryable build conflict, or a persistent copy failure.
   */
  enum class CopyResult { Success, Retry, Failed };
  CopyResult CopyModule(const std::filesystem::path& destination);

  /**
   * @brief Loads and validates a module without changing the active one.
   *
   * @param path Runtime library to test.
   * @param library Receives ownership of the loaded library.
   * @param api Receives the module's GameAPI.
   * @return true if the module and API are valid.
   */
  bool LoadCandidate(const std::filesystem::path& path, DynamicLibrary& library,
                     GameAPI& api);

  /**
   * @brief Removes runtime module copies that are no longer needed.
   */
  void CleanupRuntimeFiles();

  /**
   * @brief Validates a GameAPI returned by a game module.
   *
   * Validation ensures that the module uses the expected ABI version and
   * provides every callback required by the host.
   *
   * @param api API table to validate.
   * @return true when the API can safely be activated.
   */
  bool ValidateAPI(const GameAPI& api) const;

  /**
   * @brief Restores a previously active runtime module after reload activation
   * fails.
   *
   * The restored module receives OnAfterReload because its earlier
   * OnBeforeReload notification must be paired with a resume notification.
   *
   * @param runtimePath Runtime library path of the previous known-good module.
   * @return True when the previous module was successfully restored.
   */
  bool RestorePreviousModule(const std::filesystem::path& runtimePath);

  /**
   * @brief Promotes a validated candidate module to the active game module.
   *
   * Ownership of the candidate dynamic library is transferred directly to the
   * active module. The supplied API must already have been validated by
   * LoadCandidate().
   *
   * @param library Validated candidate dynamic library.
   * @param api Validated API exported by the candidate.
   * @param runtimePath Runtime path associated with the candidate.
   */
  void PromoteCandidate(DynamicLibrary&& library, const GameAPI& api,
                        const std::filesystem::path& runtimePath);

 private:
  DynamicLibrary m_Library;

  GameAPI m_API{};
  void* m_State = nullptr;
  HostServices m_HostServices{};
  HostContext m_HostContext{};

  InputMap m_InputMap;
  ScreenManager m_ScreenManager;

  TextureManager m_TextureManager;
  AudioManager m_AudioManager;
  ShaderManager m_ShaderManager;
  FontManager m_FontManager;

  TimeSystem m_Time;

  std::string m_AssetRoot;

  Application* m_Application = nullptr;

  /**
   * @brief Watches the compiler-produced game module for stable changes.
   *
   * The watcher handles timestamps, file sizes, and debounce timing so
   * GameModule only needs to handle module validation and replacement.
   */
  FileWatcher m_ModuleWatcher;

  std::string m_Path;

  std::filesystem::path m_RuntimePath;
  std::filesystem::path m_RuntimeDirectory;

  std::uint64_t m_RuntimeGeneration = 0;
  std::uint64_t m_ReloadCount = 0;

#if defined(_WIN32)
  // Retry one copy per interval, retaining the active module throughout.
  bool m_ReloadPending = false;
  bool m_ReportedCopyRetry = false;
  std::chrono::steady_clock::time_point m_ReloadDeadline{};
  std::chrono::steady_clock::time_point m_NextCopyAttempt{};
  std::chrono::steady_clock::time_point m_NextCleanup{};
  std::string m_LastCopyError;
#endif

  bool m_HasAPI = false;
  bool m_Started = false;

 private:
#ifdef LEVYE_WITH_SERIALIZATION
  SerializationService m_SerializationService;
  SerializationAPI m_SerializationAPI;
#endif
};
}  // namespace Levye
