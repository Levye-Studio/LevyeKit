#include "GameModule.hpp"

#include <Levye/Assets/AssetPath.hpp>
#include <Levye/Core/Application.hpp>
#include <Levye/Debug/Logger.hpp>
#include <Levye/Input/InputMap.hpp>
#include <Levye/Screen/ScreenManager.hpp>
#include <utility>

namespace Levye {
namespace {

std::string ResolveAssetPath(HostContext* host, const char* path) {
  if (!host || !path) return {};

  if (!host->assetRoot) {
    return AssetPath::Normalize(path);
  }

  return AssetPath::Resolve(*host->assetRoot, path);
}

void HostLogInfo(void* context, const char* message) {
  if (!message) return;

  Logger::Info(message);
}

void HostLogWarning(void* context, const char* message) {
  if (!message) return;

  Logger::Warning(message);
}

void HostLogError(void* context, const char* message) {
  if (!message) return;

  Logger::Error(message);
}

bool HostIsActionDown(void* context, const char* action) {
  if (!context || !action) return false;

  auto* host = static_cast<HostContext*>(context);

  if (!host->input) return false;

  return host->input->IsDown(action);
}

bool HostIsActionPressed(void* context, const char* action) {
  if (!context || !action) return false;

  auto* host = static_cast<HostContext*>(context);

  if (!host->input) return false;

  return host->input->IsPressed(action);
}

bool HostIsActionReleased(void* context, const char* action) {
  if (!context || !action) return false;

  auto* host = static_cast<HostContext*>(context);

  if (!host->input) return false;

  return host->input->IsReleased(action);
}

float HostGetAxis(void* context, const char* axis) {
  if (!context || !axis) return 0.0f;

  auto* host = static_cast<HostContext*>(context);

  if (!host->input) return 0.0f;

  return host->input->GetAxis(axis);
}

void HostBindKey(void* context, const char* action, int key) {
  if (!context || !action) return;
  auto* host = static_cast<HostContext*>(context);
  if (host->input) host->input->BindKey(action, static_cast<KeyboardKey>(key));
}

void HostBindKeyAxis(void* context, const char* axis, int negativeKey,
                     int positiveKey) {
  if (!context || !axis) return;
  auto* host = static_cast<HostContext*>(context);
  if (host->input)
    host->input->BindKeyAxis(axis, static_cast<KeyboardKey>(negativeKey),
                             static_cast<KeyboardKey>(positiveKey));
}

void HostBindGamepadButton(void* context, const char* action, int gamepad,
                           int button) {
  if (!context || !action) return;
  auto* host = static_cast<HostContext*>(context);
  if (host->input)
    host->input->BindGamepadButton(action, gamepad,
                                   static_cast<GamepadButton>(button));
}

void HostBindGamepadAxis(void* context, const char* axis, int gamepad,
                         int gamepadAxis, float deadzone) {
  if (!context || !axis) return;
  auto* host = static_cast<HostContext*>(context);
  if (host->input)
    host->input->BindGamepadAxis(
        axis, gamepad, static_cast<GamepadAxis>(gamepadAxis), deadzone);
}

void HostClearAction(void* context, const char* action) {
  if (!context || !action) return;
  auto* host = static_cast<HostContext*>(context);
  if (host->input) host->input->ClearAction(action);
}

void HostClearInput(void* context) {
  if (!context) return;
  auto* host = static_cast<HostContext*>(context);
  if (host->input) host->input->Clear();
}

void HostSetScreen(void* context, const char* screen) {
  if (!context || !screen) return;

  auto* host = static_cast<HostContext*>(context);

  if (!host->screens) return;

  host->screens->SetScreen(screen);
}

bool HostIsScreen(void* context, const char* screen) {
  if (!context || !screen) return false;

  auto* host = static_cast<HostContext*>(context);

  if (!host->screens) return false;

  return host->screens->IsScreen(screen);
}

AssetHandle HostLoadTexture(void* context, const char* path) {
  if (!context || !path) return {};

  auto* host = static_cast<HostContext*>(context);

  if (!host->textures) return {};

  const std::string resolvedPath = ResolveAssetPath(host, path);

  if (resolvedPath.empty()) return {};

  return host->textures->Load(resolvedPath);
}

const Texture2D* HostGetTexture(void* context, AssetHandle handle) {
  if (!context) return nullptr;

  auto* host = static_cast<HostContext*>(context);

  if (!host->textures) return nullptr;

  return host->textures->Get(handle);
}

AssetHandle HostLoadSound(void* context, const char* path) {
  if (!context || !path) return {};

  auto* host = static_cast<HostContext*>(context);

  if (!host->audio) return {};

  const std::string resolvedPath = ResolveAssetPath(host, path);

  return host->audio->LoadSound(resolvedPath);
}

void HostPlaySound(void* context, AssetHandle handle) {
  if (!context) return;

  auto* host = static_cast<HostContext*>(context);

  if (!host->audio) return;

  host->audio->PlaySound(handle);
}

void HostStopSound(void* context, AssetHandle handle) {
  if (!context) return;

  auto* host = static_cast<HostContext*>(context);

  if (!host->audio) return;

  host->audio->StopSound(handle);
}

void HostSetSoundVolume(void* context, AssetHandle handle, float volume) {
  if (!context) return;

  auto* host = static_cast<HostContext*>(context);

  if (!host->audio) return;

  host->audio->SetSoundVolume(handle, volume);
}

AssetHandle HostLoadMusic(void* context, const char* path) {
  if (!context || !path) return {};

  auto* host = static_cast<HostContext*>(context);

  if (!host->audio) return {};

  const std::string resolvedPath = ResolveAssetPath(host, path);

  return host->audio->LoadMusic(resolvedPath);
}

void HostPlayMusic(void* context, AssetHandle handle) {
  if (!context) return;

  auto* host = static_cast<HostContext*>(context);

  if (host->audio) host->audio->PlayMusic(handle);
}

void HostPauseMusic(void* context, AssetHandle handle) {
  if (!context) return;

  auto* host = static_cast<HostContext*>(context);

  if (host->audio) host->audio->PauseMusic(handle);
}

void HostResumeMusic(void* context, AssetHandle handle) {
  if (!context) return;

  auto* host = static_cast<HostContext*>(context);

  if (host->audio) host->audio->ResumeMusic(handle);
}

void HostStopMusic(void* context, AssetHandle handle) {
  if (!context) return;

  auto* host = static_cast<HostContext*>(context);

  if (host->audio) host->audio->StopMusic(handle);
}

void HostSetMusicVolume(void* context, AssetHandle handle, float volume) {
  if (!context) return;

  auto* host = static_cast<HostContext*>(context);

  if (host->audio) {
    host->audio->SetMusicVolume(handle, volume);
  }
}

AssetHandle HostLoadShader(void* context, const char* vertexPath,
                           const char* fragmentPath) {
  if (!context) return {};

  auto* host = static_cast<HostContext*>(context);

  if (!host->shaders) return {};

  const std::string resolvedVertex =
      vertexPath ? ResolveAssetPath(host, vertexPath) : std::string{};

  const std::string resolvedFragment =
      fragmentPath ? ResolveAssetPath(host, fragmentPath) : std::string{};

  return host->shaders->Load(resolvedVertex, resolvedFragment);
}

const Shader* HostGetShader(void* context, AssetHandle handle) {
  if (!context) return nullptr;

  auto* host = static_cast<HostContext*>(context);

  if (!host->shaders) return nullptr;

  return host->shaders->Get(handle);
}

float HostGetDeltaTime(void* context) {
  if (!context) return 0.0f;

  auto* host = static_cast<HostContext*>(context);

  if (!host->time) return 0.0f;

  return host->time->GetDeltaTime();
}

float HostGetUnscaledDeltaTime(void* context) {
  if (!context) return 0.0f;

  auto* host = static_cast<HostContext*>(context);

  if (!host->time) return 0.0f;

  return host->time->GetUnscaledDeltaTime();
}

double HostGetTime(void* context) {
  if (!context) return 0.0;

  auto* host = static_cast<HostContext*>(context);

  if (!host->time) return 0.0;

  return host->time->GetTimeSystem();
}

double HostGetUnscaledTime(void* context) {
  if (!context) return 0.0;

  auto* host = static_cast<HostContext*>(context);

  if (!host->time) return 0.0;

  return host->time->GetUnscaledTime();
}

void HostSetTimeScale(void* context, float scale) {
  if (!context) return;

  auto* host = static_cast<HostContext*>(context);

  if (host->time) host->time->SetTimeScale(scale);
}

float HostGetTimeScale(void* context) {
  if (!context) return 1.0f;

  auto* host = static_cast<HostContext*>(context);

  if (!host->time) return 1.0f;

  return host->time->GetTimeScale();
}

void HostSetPaused(void* context, bool paused) {
  if (!context) return;

  auto* host = static_cast<HostContext*>(context);

  if (host->time) host->time->SetPaused(paused);
}

bool HostIsPaused(void* context) {
  if (!context) return false;

  auto* host = static_cast<HostContext*>(context);

  if (!host->time) return false;

  return host->time->IsPaused();
}

float HostGetInterpolationAlpha(void* context) {
  if (!context) return 0.0f;

  auto* host = static_cast<HostContext*>(context);

  if (!host->time) return 0.0f;

  return host->time->GetInterpolationAlpha();
}

AssetHandle HostLoadFont(void* context, const char* path, int fontSize) {
  if (!context || !path) return {};

  auto* host = static_cast<HostContext*>(context);

  if (!host->fonts) return {};

  const std::string resolvedPath = ResolveAssetPath(host, path);

  if (resolvedPath.empty()) return {};

  return host->fonts->Load(resolvedPath, fontSize);
}

const Font* HostGetFont(void* context, AssetHandle handle) {
  if (!context) return nullptr;

  auto* host = static_cast<HostContext*>(context);

  if (!host->fonts) return nullptr;

  return host->fonts->Get(handle);
}

/**
 * @brief Resolves an optional module's public function table.
 *
 * Returns nullptr if the module is unavailable or the requested
 * API version is unsupported.
 */
const void* HostGetModuleAPI(void* context, ModuleID module,
                             ModuleAPIVersion version) {
  if (!context) return nullptr;

  auto* host = static_cast<HostContext*>(context);

  switch (module) {
    case ModuleID::Serialization:
#ifdef LEVYE_WITH_SERIALIZATION
      if (version != SERIALIZATION_API_VERSION) return nullptr;

      return host->serializationAPI;
#else
      return nullptr;
#endif
  }

  return nullptr;
}
bool g_QuitRequested = false;

void HostRequestQuit(void* context) {
  if (!context) return;

  auto* host = static_cast<HostContext*>(context);

  if (!host->application) return;

  host->application->RequestQuit();
}

}  // namespace
GameModule::~GameModule() {
  Unload();
  CleanupRuntimeFiles();
}

bool GameModule::Load(const std::string& path) {
  Unload();

#if defined(_WIN32)
  CleanupRuntimeFiles();
  m_Path = std::filesystem::absolute(path).string();
#else
  m_Path = path;
#endif

#ifdef LEVYE_WITH_SERIALIZATION
  m_SerializationAPI = MakeSerializationAPI(m_SerializationService);
#endif

  m_HostContext = {.input = &m_InputMap,
                   .screens = &m_ScreenManager,
                   .textures = &m_TextureManager,
                   .audio = &m_AudioManager,
                   .shaders = &m_ShaderManager,
                   .fonts = &m_FontManager,
#ifdef LEVYE_WITH_SERIALIZATION
                   .serialization = &m_SerializationService,
                   .serializationAPI = &m_SerializationAPI,
#endif
                   .time = &m_Time,
                   .assetRoot = &m_AssetRoot,
                   .application = m_Application};

  m_HostServices = {.context = &m_HostContext,

                    .LogInfo = HostLogInfo,
                    .LogWarning = HostLogWarning,
                    .LogError = HostLogError,

                    .IsActionDown = HostIsActionDown,
                    .IsActionPressed = HostIsActionPressed,
                    .IsActionReleased = HostIsActionReleased,

                    .GetAxis = HostGetAxis,
                    .BindKey = HostBindKey,
                    .BindKeyAxis = HostBindKeyAxis,
                    .BindGamepadButton = HostBindGamepadButton,
                    .BindGamepadAxis = HostBindGamepadAxis,
                    .ClearAction = HostClearAction,
                    .ClearInput = HostClearInput,

                    .SetScreen = HostSetScreen,
                    .IsScreen = HostIsScreen,

                    .LoadTexture = HostLoadTexture,
                    .GetTexture = HostGetTexture,

                    .LoadSound = HostLoadSound,
                    .PlaySound = HostPlaySound,
                    .StopSound = HostStopSound,
                    .SetSoundVolume = HostSetSoundVolume,

                    .LoadMusic = HostLoadMusic,
                    .PlayMusic = HostPlayMusic,
                    .PauseMusic = HostPauseMusic,
                    .ResumeMusic = HostResumeMusic,
                    .StopMusic = HostStopMusic,
                    .SetMusicVolume = HostSetMusicVolume,

                    .LoadShader = HostLoadShader,
                    .GetShader = HostGetShader,

                    .LoadFont = HostLoadFont,
                    .GetFont = HostGetFont,

                    .GetDeltaTime = HostGetDeltaTime,
                    .GetUnscaledDeltaTime = HostGetUnscaledDeltaTime,

                    .GetTime = HostGetTime,
                    .GetUnscaledTime = HostGetUnscaledTime,

                    .SetTimeScale = HostSetTimeScale,
                    .GetTimeScale = HostGetTimeScale,

                    .SetPaused = HostSetPaused,
                    .IsPaused = HostIsPaused,
                    .GetInterpolationAlpha = HostGetInterpolationAlpha,
                    .GetModuleAPI = HostGetModuleAPI,
                    .RequestQuit = HostRequestQuit};

  const std::filesystem::path sourcePath(m_Path);

  if (!std::filesystem::exists(sourcePath)) {
    Logger::Error("Game module does not exist: " + m_Path);

    return false;
  }

  if (!m_ModuleWatcher.Watch(m_Path)) {
    Logger::Warning("Failed to watch game module: " + m_Path);
  }

  /*
   * Keep temporary libraries away from the actual build output.
   *
   * Example:
   *
   * Targets/Debug/lib/Game.dylib
   * Targets/Debug/hotreload/Game_1.dylib
   * Windows isolates DLL copies in a per-host subdirectory.
   */
  const auto runtimeRoot = sourcePath.parent_path().parent_path() / "hotreload";

  std::error_code error;

  std::filesystem::create_directories(runtimeRoot, error);

  if (error) {
    Logger::Error("Failed to create hot-reload directory: " + error.message());

    return false;
  }

#if defined(_WIN32)
  // Atomically reserve an exclusive session directory. A second host or a
  // previous crashed session must never share our loaded DLL filenames.
  const auto session =
      std::chrono::system_clock::now().time_since_epoch().count();
  for (std::uint64_t suffix = 0;; ++suffix) {
    m_RuntimeDirectory =
        runtimeRoot / (sourcePath.stem().string() + "_" +
                       std::to_string(session) + "_" + std::to_string(suffix));
    if (std::filesystem::create_directory(m_RuntimeDirectory, error)) break;
    if (error) {
      Logger::Error("Failed to reserve runtime directory: " + error.message());
      m_RuntimeDirectory.clear();
      return false;
    }
  }
#else
  // Keep the established dylib location, including loader-relative paths.
  m_RuntimeDirectory = runtimeRoot;
  CleanupRuntimeFiles();
#endif

  const auto runtimePath = CreateRuntimePath();

  if (CopyModule(runtimePath) != CopyResult::Success) {
#if defined(_WIN32)
    Logger::Error("Failed to copy initial game module: " + m_LastCopyError);
    CleanupRuntimeFiles();
#endif
    return false;
  }

  DynamicLibrary candidateLibrary;
  GameAPI candidateAPI{};

  if (!LoadCandidate(runtimePath, candidateLibrary, candidateAPI)) {
    std::filesystem::remove(runtimePath, error);

    return false;
  }

  PromoteCandidate(std::move(candidateLibrary), candidateAPI, runtimePath);

  m_Started = false;

  Logger::Info("Loaded game module: " + m_Path);

  return true;
}

bool GameModule::Start() {
  if (!m_HasAPI) {
    Logger::Error("Cannot start game module: no valid GameAPI is loaded.");

    return false;
  }

  if (m_Started) return true;

  if (!m_API.BindServices) return false;

  m_API.BindServices(&m_HostServices);

  m_State = ::operator new(m_API.stateSize);

  m_API.InitializeState(m_State);

  if (m_API.OnLoad) {
    m_API.OnLoad(m_State);
  }

  m_Started = true;

  Logger::Info("Game module started.");

  return true;
}

bool GameModule::CheckForReload() {
  if (!m_Started || m_Path.empty()) return false;

  /*
   * FileWatcher owns all filesystem polling and debounce logic.
   * A true result means the compiler output changed and remained stable
   * long enough for us to safely attempt a reload.
   */
#if defined(_WIN32)
  const auto now = std::chrono::steady_clock::now();
  if (now >= m_NextCleanup) {
    CleanupRuntimeFiles();
    m_NextCleanup = now + std::chrono::seconds(1);
  }

  if (m_ModuleWatcher.Poll()) {
    Logger::Info("Stable game module change detected.");
    m_ReloadPending = true;
    m_ReportedCopyRetry = false;
    m_LastCopyError = "Build output did not remain stable.";
    m_ReloadDeadline = now + std::chrono::seconds(5);
    m_NextCopyAttempt = now;
  }

  if (!m_ReloadPending) return false;

  if (now >= m_ReloadDeadline) {
    Logger::Error(
        "Hot reload copy timed out after 5 seconds: " + m_LastCopyError +
        " Keeping current module; rebuild to retry.");
    m_ReloadPending = false;
    return false;
  }

  if (now < m_NextCopyAttempt || !m_ModuleWatcher.IsCurrentStateStable())
    return false;

  const auto candidatePath = CreateRuntimePath();

  const auto copyResult = CopyModule(candidatePath);
  if (copyResult == CopyResult::Retry) {
    if (!m_ReportedCopyRetry) {
      Logger::Warning(
          "Game module temporarily unavailable: " + m_LastCopyError +
          " Retrying across frames; keeping current module.");
      m_ReportedCopyRetry = true;
    }
    m_NextCopyAttempt = now + std::chrono::milliseconds(100);
    return false;
  }
  m_ReloadPending = false;
  if (copyResult == CopyResult::Failed) {
    Logger::Error("Failed to copy reload candidate: " + m_LastCopyError +
                  " Keeping current module.");
    return false;
  }
#else
  if (!m_ModuleWatcher.Poll()) return false;

  Logger::Info("Stable game module change detected.");
  const auto candidatePath = CreateRuntimePath();
  if (CopyModule(candidatePath) != CopyResult::Success) {
    Logger::Warning("Could not copy reload candidate. Keeping current module.");
    return false;
  }
#endif

  DynamicLibrary candidateLibrary;
  GameAPI candidateAPI{};

  /*
   * Validate the replacement while the currently running module is still
   * completely intact.
   */
  if (!LoadCandidate(candidatePath, candidateLibrary, candidateAPI)) {
    Logger::Warning("Reload candidate is invalid. Keeping current module.");

    std::error_code error;
    std::filesystem::remove(candidatePath, error);

    return false;
  }

  if (m_State && candidateAPI.stateSize != m_API.stateSize) {
    Logger::Warning(
        "Hot reload rejected because GameState size changed. "
        "Restart the game to apply the new state layout.");

    candidateLibrary.Unload();

    std::error_code error;

    std::filesystem::remove(candidatePath, error);

    return false;
  }

  const std::filesystem::path previousRuntimePath = m_RuntimePath;

  /*
   * The candidate is valid, so the old game code can now prepare for module
   * replacement without risking interruption from an invalid build.
   */

  m_API.OnBeforeReload(m_State);

  m_API = {};
  m_HasAPI = false;

  m_Library.Unload();

  PromoteCandidate(std::move(candidateLibrary), candidateAPI, candidatePath);

  m_API.BindServices(&m_HostServices);
  /*
   * The new game code is now active and can resume using the persistent
   * host-owned GameState and services.
   */

  m_API.OnAfterReload(m_State);

  /*
   * The previous runtime copy is no longer executable and can now be removed.
   */
  if (!previousRuntimePath.empty()) {
    std::error_code error;

    std::filesystem::remove(previousRuntimePath, error);
  }

  ++m_ReloadCount;

  Logger::Info("Hot reload successful. Reload #" +
               std::to_string(m_ReloadCount));

  return true;
}

void GameModule::PromoteCandidate(DynamicLibrary&& library, const GameAPI& api,
                                  const std::filesystem::path& runtimePath) {
  /*
   * Transfer ownership of the already validated native library handle.
   * This guarantees that the module becoming active is the exact module
   * whose API was checked by LoadCandidate().
   */
  m_Library = std::move(library);

  m_API = api;
  m_HasAPI = true;
  m_RuntimePath = runtimePath;
}

void GameModule::Update(float deltaTime) {
  if (!m_HasAPI || !m_API.OnUpdate) return;

  m_API.OnUpdate(m_State, deltaTime);
}

void GameModule::UpdateTime(float deltaTime) { m_Time.Update(deltaTime); }

void GameModule::FixedUpdate(float fixedDeltaTime) {
  if (!m_HasAPI || !m_API.OnFixedUpdate) {
    return;
  }

  m_API.OnFixedUpdate(m_State, fixedDeltaTime);
}

void GameModule::Draw() {
  if (!m_HasAPI || !m_API.OnDraw) return;

  m_API.OnDraw(m_State);
}

void GameModule::Unload() {
#if defined(_WIN32)
  m_ReloadPending = false;
#endif
  if (!m_Library.IsLoaded()) return;

  m_API = {};
  m_HasAPI = false;
  m_Started = false;

  m_Library.Unload();

  std::error_code error;

  if (!m_RuntimePath.empty()) {
    std::filesystem::remove(m_RuntimePath, error);

    m_RuntimePath.clear();
  }

  Logger::Info("Game module unloaded.");
}

void GameModule::Shutdown() {
  if (!m_Library.IsLoaded()) return;

  if (m_State) {
    if (m_API.OnShutdown) {
      m_API.OnShutdown(m_State);
    }

    if (m_API.DestroyState) {
      m_API.DestroyState(m_State);
    }

    ::operator delete(m_State);

    m_State = nullptr;
  }

  /*
   * The game module no longer needs access to host-owned systems.
   * Clear its module-local Services binding before unloading the library.
   */
  if (m_API.BindServices) {
    m_API.BindServices(nullptr);
  }

  Unload();
}

bool GameModule::IsLoaded() const { return m_Library.IsLoaded() && m_HasAPI; }

const std::string& GameModule::GetPath() const { return m_Path; }

void GameModule::UpdateHostSystems() {
  m_TextureManager.CheckForChanges();
  m_ShaderManager.CheckForChanges();
  m_FontManager.CheckForChanges();

  m_AudioManager.CheckForChanges();
  /*
   * Streaming music requires regular buffer updates. Keeping this in the
   * host means playback continues across game-code hot reloads.
   */
  m_AudioManager.Update();
}

std::filesystem::path GameModule::CreateRuntimePath() {
  ++m_RuntimeGeneration;

  const std::filesystem::path sourcePath(m_Path);

  const std::string filename = sourcePath.stem().string() + "_" +
                               std::to_string(m_RuntimeGeneration) +
                               sourcePath.extension().string();

  return m_RuntimeDirectory / filename;
}

GameModule::CopyResult GameModule::CopyModule(
    const std::filesystem::path& destination) {
  std::error_code error;

#if defined(_WIN32)
  std::filesystem::copy_file(m_Path, destination,
                             std::filesystem::copy_options::none, error);

  CopyResult result = CopyResult::Success;
  if (error) {
    m_LastCopyError =
        error.message() + " (" + std::to_string(error.value()) + ")";
    result = CopyResult::Failed;
    // Implementations can map native sharing/lock violations (32/33) to
    // permission_denied. Bound retries to also diagnose persistent ACL errors.
    if ((error.category() == std::system_category() &&
         (error.value() == 32 || error.value() == 33)) ||
        error == std::errc::permission_denied ||
        error == std::errc::no_such_file_or_directory)
      result = CopyResult::Retry;
  } else if (!m_ModuleWatcher.IsCurrentStateStable()) {
    m_LastCopyError = "Build output changed during the copy.";
    result = CopyResult::Retry;
  }

  if (result != CopyResult::Success) {
    std::error_code cleanupError;
    std::filesystem::remove(destination, cleanupError);
  }
  return result;
#else
  std::filesystem::copy_file(m_Path, destination,
                             std::filesystem::copy_options::overwrite_existing,
                             error);
  if (error) {
    Logger::Error("Failed to copy game module: " + error.message());
    return CopyResult::Failed;
  }
  return CopyResult::Success;
#endif
}

bool GameModule::LoadCandidate(const std::filesystem::path& path,
                               DynamicLibrary& library, GameAPI& api) {
  if (!library.Load(path.string())) return false;

  void* symbol = library.GetSymbol("GetGameAPI");

  if (!symbol) {
    library.Unload();
    return false;
  }

  auto getGameAPI = reinterpret_cast<GetGameAPIFn>(symbol);

  const GameAPI* gameAPI = getGameAPI();

  if (!gameAPI) {
    Logger::Error("Game module returned a null GameAPI.");

    library.Unload();
    return false;
  }

  api = *gameAPI;

  if (!ValidateAPI(api)) {
    library.Unload();
    return false;
  }

  return true;
}

void GameModule::CleanupRuntimeFiles() {
  if (m_RuntimeDirectory.empty()) return;

  std::error_code error;

  if (!std::filesystem::exists(m_RuntimeDirectory, error)) {
    return;
  }

#if defined(_WIN32)
  /*
   * Only clean this instance's reserved directory. Other hosts may still be
   * executing their copies. Failed deletions are retried by the next sweep.
   */
  for (auto entry =
           std::filesystem::directory_iterator(m_RuntimeDirectory, error);
       !error && entry != std::filesystem::directory_iterator{};
       entry.increment(error)) {
    std::error_code fileError;
    if (entry->path() == m_RuntimePath || !entry->is_regular_file(fileError))
      continue;

    std::filesystem::remove(entry->path(), fileError);
  }
  // Keep the session directory reserved until the module has been unloaded.
  if (!m_Library.IsLoaded()) std::filesystem::remove(m_RuntimeDirectory, error);
#else
  // Preserve the existing macOS/Linux startup and shutdown cleanup policy.
  for (const auto& entry :
       std::filesystem::directory_iterator(m_RuntimeDirectory, error)) {
    if (error) break;
    if (!entry.is_regular_file()) continue;
    std::filesystem::remove(entry.path(), error);
    error.clear();
  }
#endif
}

bool GameModule::ValidateAPI(const GameAPI& api) const {
  if (api.version != GAME_API_VERSION) {
    Logger::Error(
        "Game API version mismatch. Host: " + std::to_string(GAME_API_VERSION) +
        ", Game: " + std::to_string(api.version));

    return false;
  }

  if (api.stateSize == 0) {
    Logger::Error("Game module reported an invalid state size.");

    return false;
  }

  if (!api.BindServices || !api.InitializeState || !api.DestroyState ||
      !api.OnLoad || !api.OnBeforeReload || !api.OnAfterReload ||
      !api.OnUpdate || !api.OnFixedUpdate || !api.OnDraw || !api.OnShutdown) {
    Logger::Error("Game module is missing required callbacks.");

    return false;
  }

  return true;
}

TimeSystem& GameModule::GetTimeSystem() { return m_Time; }

InputMap& GameModule::GetInputMap() { return m_InputMap; }

ScreenManager& GameModule::GetScreenManager() { return m_ScreenManager; }

void GameModule::SetAssetRoot(const std::string& path) {
  m_AssetRoot = AssetPath::Normalize(path);
}

const std::string& GameModule::GetAssetRoot() const { return m_AssetRoot; }

void GameModule::ReleaseResources() {
  m_AudioManager.Clear();

  m_FontManager.Clear();
  m_ShaderManager.Clear();
  m_TextureManager.Clear();
}

void GameModule::SetApplication(Application* application) {
  m_Application = application;
  m_HostContext.application = application;
}

}  // namespace Levye
