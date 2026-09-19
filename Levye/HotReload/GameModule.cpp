#include "GameModule.hpp"

#include <iostream>

namespace Levye {
GameModule::~GameModule() { Unload(); }

bool GameModule::Load(const std::string &path) {
  // Function pointers from an older module must never survive while a
  // different dynamic library is being loaded.
  Unload();

  m_Path = path;

  if (!std::filesystem::exists(m_Path)) {
    std::cerr << "[LevyeKit] Game module does not exist: " << m_Path << '\n';

    return false;
  }

  if (!LoadLibrary())
    return false;

  m_LastWriteTime = std::filesystem::last_write_time(m_Path);

  m_Started = true;

  if (m_API.OnLoad)
    m_API.OnLoad(&m_State);

  std::cout << "[LevyeKit] Loaded game module: " << m_Path << '\n';

  return true;
}

bool GameModule::LoadLibrary() {

  if (!m_Library.Load(m_Path))
    return false;

  void *symbol = m_Library.GetSymbol("GetGameAPI");

  if (!symbol) {
    std::cerr << "[LevyeKit] Game module does not export "
              << "GetGameAPI.\n";

    m_Library.Unload();

    return false;
  }

  auto getGameAPI = reinterpret_cast<GetGameAPIFn>(symbol);

  GameAPI api = getGameAPI();

  // Update and Draw form the minimum usable runtime API for a game
  // module. Load and Unload remain optional lifecycle callbacks.
  if (!api.OnUpdate || !api.OnDraw) {
    std::cerr << "[LevyeKit] Game module returned an invalid "
              << "GameAPI.\n";

    m_Library.Unload();

    return false;
  }
  m_API = api;
  m_HasAPI = true;

  return true;
}

bool GameModule::CheckForReload() {
  if (!m_Started || m_Path.empty())
    return false;

  if (!std::filesystem::exists(m_Path))
    return false;

  const auto currentWriteTime = std::filesystem::last_write_time(m_Path);

  if (currentWriteTime == m_LastWriteTime)
    return false;

  std::cout << "[LevyeKit] Game module change detected.\n";

  /*
   * Record the timestamp now so a failed reload does not cause LevyeKit
   * to attempt loading the same broken build every frame.
   */
  m_LastWriteTime = currentWriteTime;

  if (m_API.OnUnload)
    m_API.OnUnload(&m_State);

  // Function pointers must be discarded before unloading the machine
  // code that owns them.
  m_API = {};
  m_HasAPI = false;

  m_Library.Unload();

  if (!LoadLibrary()) {
    std::cerr << "[LevyeKit] Failed to reload game module.\n";

    return false;
  }

  ++m_State.reloadCount;

  if (m_API.OnReload)
    m_API.OnReload(&m_State);

  std::cout << "[LevyeKit] Game module reloaded successfully. Reload #"
            << m_State.reloadCount << '\n';

  return true;
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

  // Clear pointers before removing the machine code they point into.
  m_API = {};
  m_HasAPI = false;
  m_Started = false;

  m_Library.Unload();

  std::cout << "[LevyeKit] Game module unloaded.\n";
}

bool GameModule::IsLoaded() const { return m_Library.IsLoaded() && m_HasAPI; }

const std::string &GameModule::GetPath() const { return m_Path; }
} // namespace Levye