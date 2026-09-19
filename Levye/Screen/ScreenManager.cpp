#include "ScreenManager.hpp"

namespace Levye {
void ScreenManager::SetScreen(const std::string &name) {
  m_CurrentScreen = name;
}

const std::string &ScreenManager::GetScreen() const { return m_CurrentScreen; }

bool ScreenManager::IsScreen(const std::string &name) const {
  return m_CurrentScreen == name;
}

bool ScreenManager::HasScreen() const { return !m_CurrentScreen.empty(); }

void ScreenManager::Clear() { m_CurrentScreen.clear(); }
} // namespace Levye