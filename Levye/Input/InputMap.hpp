#pragma once

#include <raylib.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace Levye {
/**
 * @brief Stores named input actions and their physical bindings.
 *
 * InputMap allows game code to query logical actions such as "Jump" or
 * "MoveLeft" without depending directly on a specific keyboard key or
 * gamepad button.
 *
 * InputMap is owned by the host and therefore survives game-code reloads.
 */
class InputMap {
public:
  /**
   * @brief Adds a keyboard binding to an action.
   *
   * The action is created automatically if it does not already exist.
   *
   * @param action Logical action name.
   * @param key Keyboard key associated with the action.
   */
  void BindKey(const std::string &action, KeyboardKey key);

  /**
   * @brief Adds a gamepad button binding to an action.
   *
   * @param action Logical action name.
   * @param gamepad Gamepad index.
   * @param button Gamepad button associated with the action.
   */
  void BindGamepadButton(const std::string &action, int gamepad,
                         GamepadButton button);

  /**
   * @brief Returns whether any binding for an action is currently held.
   *
   * @param action Logical action name.
   */
  bool IsDown(const std::string &action) const;

  /**
   * @brief Returns whether any binding for an action was pressed this frame.
   *
   * @param action Logical action name.
   */
  bool IsPressed(const std::string &action) const;

  /**
   * @brief Returns whether any binding for an action was released this frame.
   *
   * @param action Logical action name.
   */
  bool IsReleased(const std::string &action) const;

  /**
   * @brief Removes every binding associated with an action.
   *
   * @param action Logical action name.
   */
  void ClearAction(const std::string &action);

  /**
   * @brief Removes all registered actions and bindings.
   */
  void Clear();

private:
  struct GamepadButtonBinding {
    int gamepad = 0;
    GamepadButton button = GAMEPAD_BUTTON_UNKNOWN;
  };

  struct Action {
    std::vector<KeyboardKey> keys;
    std::vector<GamepadButtonBinding> gamepadButtons;
  };

  std::unordered_map<std::string, Action> m_Actions;
};
} // namespace Levye