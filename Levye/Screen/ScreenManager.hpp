#pragma once

#include <string>

namespace Levye {
/**
 * @brief Tracks the currently active logical game screen.
 *
 * ScreenManager is owned by the host so the active screen remains
 * unchanged when reloadable game code is replaced.
 *
 * The manager intentionally stores only screen identity. Rendering,
 * updating and screen-specific state remain responsibilities of the game.
 */
class ScreenManager {
public:
  /**
   * @brief Changes the currently active screen.
   *
   * @param name Logical name of the new screen.
   */
  void SetScreen(const std::string &name);

  /**
   * @brief Returns the name of the currently active screen.
   */
  const std::string &GetScreen() const;

  /**
   * @brief Returns whether the requested screen is currently active.
   *
   * @param name Screen name to compare against.
   */
  bool IsScreen(const std::string &name) const;

  /**
   * @brief Returns whether an active screen has been assigned.
   */
  bool HasScreen() const;

  /**
   * @brief Removes the currently active screen.
   */
  void Clear();

private:
  std::string m_CurrentScreen;
};
} // namespace Levye