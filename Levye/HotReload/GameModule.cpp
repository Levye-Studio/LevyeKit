#include "GameModule.hpp"

#include <iostream>

namespace Levye {
GameModule::~GameModule() { Unload(); }

bool GameModule::Load(const std::string &path) {
  // Function pointers from an older module must never survive while a
  // different dynamic library is being loaded.
  Unload();

  if (!m_Library.Load(path))
    return false;

  void *symbol = m_Library.GetSymbol("GetGameAPI");

  if (!symbol) {
    std::cerr << "[LevyeKit] Game module does not export "
              << "GetGameAPI.\n";

    m_Library.Unload();

    return false;
  }

  auto getGameAPI = reinterpret_cast<GetGameAPIFn>(symbol);

  m_API = getGameAPI();

  // Update and Draw form the minimum usable runtime API for a game
  // module. Load and Unload remain optional lifecycle callbacks.
  if (!m_API.OnUpdate || !m_API.OnDraw) {
    std::cerr << "[LevyeKit] Game module returned an invalid "
              << "GameAPI.\n";

    m_API = {};

    m_Library.Unload();

    return false;
  }

  m_Path = path;
  m_HasAPI = true;

  std::cout << "[LevyeKit] Loaded game module: " << path << '\n';

  return true;
}

void GameModule::Unload() {
  if (!m_Library.IsLoaded())
    return;

  // Clear pointers before removing the machine code they point into.
  m_API = {};
  m_HasAPI = false;

  m_Library.Unload();

  std::cout << "[LevyeKit] Game module unloaded.\n";
}

const GameAPI *GameModule::GetAPI() const {
  if (!m_HasAPI)
    return nullptr;

  return &m_API;
}

bool GameModule::IsLoaded() const { return m_Library.IsLoaded() && m_HasAPI; }

const std::string &GameModule::GetPath() const { return m_Path; }
} // namespace Levye