#include "GameModule.hpp"

#include <iostream>

namespace Levye {
GameModule::~GameModule() {
  Unload();
  CleanupRuntimeFiles();
}

bool GameModule::Load(const std::string &path) {
  Unload();

  m_Path = path;

  const std::filesystem::path sourcePath(m_Path);

  if (!std::filesystem::exists(sourcePath)) {
    std::cerr << "[LevyeKit] Game module does not exist: " << m_Path << '\n';

    return false;
  }

  /*
   * Keep temporary libraries away from the actual build output.
   *
   * Example:
   *
   * Targets/Debug/lib/Game.dylib
   * Targets/Debug/hotreload/Game_1.dylib
   */
  m_RuntimeDirectory = sourcePath.parent_path().parent_path() / "hotreload";

  std::error_code error;

  std::filesystem::create_directories(m_RuntimeDirectory, error);

  if (error) {
    std::cerr << "[LevyeKit] Failed to create hot-reload directory: "
              << error.message() << '\n';

    return false;
  }

  CleanupRuntimeFiles();

  const auto runtimePath = CreateRuntimePath();

  if (!CopyModule(runtimePath))
    return false;

  DynamicLibrary candidateLibrary;
  GameAPI candidateAPI{};

  if (!LoadCandidate(runtimePath, candidateLibrary, candidateAPI)) {
    std::filesystem::remove(runtimePath, error);

    return false;
  }

  /*
   * DynamicLibrary is intentionally non-copyable, so for the first
   * version we cannot transfer candidateLibrary into m_Library yet.
   *
   * Validate the candidate, unload it, then load the exact same runtime
   * file into the active library.
   */
  candidateLibrary.Unload();

  if (!m_Library.Load(runtimePath.string()))
    return false;

  void *symbol = m_Library.GetSymbol("GetGameAPI");

  if (!symbol) {
    m_Library.Unload();
    return false;
  }

  auto getGameAPI = reinterpret_cast<GetGameAPIFn>(symbol);

  m_API = getGameAPI();

  m_RuntimePath = runtimePath;

  m_LastWriteTime = std::filesystem::last_write_time(sourcePath);

  m_PendingWriteTime = {};
  m_ReloadPending = false;

  m_HasAPI = true;
  m_Started = true;

  if (m_API.OnLoad)
    m_API.OnLoad(&m_State);

  std::cout << "[LevyeKit] Loaded game module: " << m_Path << '\n';

  return true;
}

bool GameModule::CheckForReload() {
  if (!m_Started || m_Path.empty())
    return false;

  const std::filesystem::path sourcePath(m_Path);

  if (!std::filesystem::exists(sourcePath))
    return false;

  std::error_code error;

  const auto currentWriteTime =
      std::filesystem::last_write_time(sourcePath, error);

  if (error)
    return false;

  const auto now = std::chrono::steady_clock::now();

  /*
   * A new timestamp means the compiler or linker has modified the game
   * module. Do not reload immediately because the file may still be in the
   * process of being written.
   */
  if (currentWriteTime != m_LastWriteTime) {
    /*
     * If this is the first observed change, or the file changed again
     * while we were waiting, restart the stability timer.
     */
    if (!m_ReloadPending || currentWriteTime != m_PendingWriteTime) {
      m_PendingWriteTime = currentWriteTime;

      m_ChangeDetectedAt = now;

      m_ReloadPending = true;

      return false;
    }

    /*
     * The timestamp has remained unchanged, but we still need to wait for
     * the debounce period before treating the build as complete.
     */
    if (now - m_ChangeDetectedAt < ReloadDebounce)
      return false;

    std::cout << "[LevyeKit] Stable game module change detected.\n";

    /*
     * Record this build before attempting the reload. A broken binary
     * should not be retried every frame.
     */
    m_LastWriteTime = currentWriteTime;

    m_ReloadPending = false;

    const auto candidatePath = CreateRuntimePath();

    if (!CopyModule(candidatePath)) {
      std::cerr << "[LevyeKit] Could not copy reload candidate. "
                << "Keeping current module.\n";

      return false;
    }

    DynamicLibrary candidateLibrary;
    GameAPI candidateAPI{};

    /*
     * Validate the candidate while the current game module is still
     * completely intact.
     */
    if (!LoadCandidate(candidatePath, candidateLibrary, candidateAPI)) {
      std::cerr << "[LevyeKit] Reload candidate is invalid. "
                << "Keeping current module.\n";

      std::filesystem::remove(candidatePath, error);

      return false;
    }

    /*
     * Validation succeeded. From this point onward we can safely begin
     * replacing the currently active module.
     */
    if (m_API.OnUnload)
      m_API.OnUnload(&m_State);

    m_API = {};
    m_HasAPI = false;

    m_Library.Unload();

    const auto previousRuntimePath = m_RuntimePath;

    /*
     * Release the validation handle. The same runtime copy will now be
     * opened as the active game module.
     */
    candidateLibrary.Unload();

    if (!m_Library.Load(candidatePath.string())) {
      std::cerr << "[LevyeKit] Failed to activate reload candidate.\n";

      /*
       * Activation failed after validation. Attempt to restore the
       * previous known-good runtime module.
       */
      if (!previousRuntimePath.empty() &&
          m_Library.Load(previousRuntimePath.string())) {
        void *oldSymbol = m_Library.GetSymbol("GetGameAPI");

        if (oldSymbol) {
          auto oldGetGameAPI = reinterpret_cast<GetGameAPIFn>(oldSymbol);

          m_API = oldGetGameAPI();
          m_HasAPI = true;

          std::cerr << "[LevyeKit] Previous module restored.\n";
        }
      }

      return false;
    }

    void *symbol = m_Library.GetSymbol("GetGameAPI");

    if (!symbol) {
      std::cerr << "[LevyeKit] Activated module lost GetGameAPI.\n";

      m_Library.Unload();

      return false;
    }

    auto getGameAPI = reinterpret_cast<GetGameAPIFn>(symbol);

    m_API = getGameAPI();
    m_HasAPI = true;

    m_RuntimePath = candidatePath;

    ++m_State.reloadCount;

    if (m_API.OnReload)
      m_API.OnReload(&m_State);

    /*
     * The previous runtime library is no longer executing and can now be
     * safely removed.
     */
    if (!previousRuntimePath.empty()) {
      std::filesystem::remove(previousRuntimePath, error);
    }

    std::cout << "[LevyeKit] Hot reload successful. Reload #"
              << m_State.reloadCount << '\n';

    return true;
  }

  /*
   * If the file returned to the timestamp of the active build, there is
   * nothing left waiting to be reloaded.
   */
  m_ReloadPending = false;

  return false;
}

void GameModule::Update(float deltaTime) {
  if (!m_HasAPI || !m_API.OnUpdate)
    return;

  m_API.OnUpdate(&m_State, deltaTime);
}

void GameModule::Draw() {
  if (!m_HasAPI || !m_API.OnDraw)
    return;

  m_API.OnDraw(&m_State);
}

void GameModule::Unload() {
  if (!m_Library.IsLoaded())
    return;

  if (m_HasAPI && m_API.OnUnload)
    m_API.OnUnload(&m_State);

  m_API = {};
  m_HasAPI = false;
  m_Started = false;
  m_ReloadPending = false;
  m_PendingWriteTime = {};

  m_Library.Unload();

  std::error_code error;

  if (!m_RuntimePath.empty()) {
    std::filesystem::remove(m_RuntimePath, error);

    m_RuntimePath.clear();
  }

  std::cout << "[LevyeKit] Game module unloaded.\n";
}

bool GameModule::IsLoaded() const { return m_Library.IsLoaded() && m_HasAPI; }

const std::string &GameModule::GetPath() const { return m_Path; }

std::filesystem::path GameModule::CreateRuntimePath() {
  ++m_RuntimeGeneration;

  const std::filesystem::path sourcePath(m_Path);

  const std::string filename = sourcePath.stem().string() + "_" +
                               std::to_string(m_RuntimeGeneration) +
                               sourcePath.extension().string();

  return m_RuntimeDirectory / filename;
}

bool GameModule::CopyModule(const std::filesystem::path &destination) {
  std::error_code error;

  std::filesystem::copy_file(m_Path, destination,
                             std::filesystem::copy_options::overwrite_existing,
                             error);

  if (error) {
    std::cerr << "[LevyeKit] Failed to copy game module: " << error.message()
              << '\n';

    return false;
  }

  return true;
}

bool GameModule::LoadCandidate(const std::filesystem::path &path,
                               DynamicLibrary &library, GameAPI &api) {
  if (!library.Load(path.string()))
    return false;

  void *symbol = library.GetSymbol("GetGameAPI");

  if (!symbol) {
    library.Unload();
    return false;
  }

  auto getGameAPI = reinterpret_cast<GetGameAPIFn>(symbol);

  api = getGameAPI();

  if (!api.OnUpdate || !api.OnDraw) {
    std::cerr << "[LevyeKit] Candidate returned an invalid GameAPI.\n";

    library.Unload();

    return false;
  }

  return true;
}

void GameModule::CleanupRuntimeFiles() {
  if (m_RuntimeDirectory.empty())
    return;

  std::error_code error;

  if (!std::filesystem::exists(m_RuntimeDirectory, error)) {
    return;
  }

  /*
   * Runtime libraries are temporary build artifacts. Remove leftovers
   * from previous runs or crashes before starting a new session.
   */
  for (const auto &entry :
       std::filesystem::directory_iterator(m_RuntimeDirectory, error)) {
    if (error)
      break;

    if (!entry.is_regular_file())
      continue;

    std::filesystem::remove(entry.path(), error);

    error.clear();
  }
}
} // namespace Levye