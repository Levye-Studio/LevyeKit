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
   * @brief Adds a pair of keyboard keys to a named axis.
   *
   * The negative key produces -1 and the positive key produces +1.
   *
   * @param axis Logical axis name.
   * @param negativeKey Key representing the negative direction.
   * @param positiveKey Key representing the positive direction.
   */
  void BindKeyAxis(const std::string &axis, KeyboardKey negativeKey,
                   KeyboardKey positiveKey);

  /**
   * @brief Adds a gamepad analog axis to a named logical axis.
   *
   * @param axis Logical axis name.
   * @param gamepad Gamepad index.
   * @param gamepadAxis Native raylib gamepad axis.
   * @param deadzone Values smaller than this amount are treated as zero.
   */
  void BindGamepadAxis(const std::string &axis, int gamepad,
                       GamepadAxis gamepadAxis, float deadzone = 0.15f);

  /**
   * @brief Returns the current value of a named logical axis.
   *
   * Keyboard bindings produce values between -1 and +1. Analog bindings
   * preserve their analog magnitude after deadzone processing.
   *
   * @param axis Logical axis name.
   * @return Axis value in the range -1 to +1.
   */
  float GetAxis(const std::string &axis) const;

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
   * @brief Removes all registered actions, axes and bindings.
   */
  void Clear();

private:
  struct KeyAxisBinding {
    KeyboardKey negativeKey = KEY_NULL;
    KeyboardKey positiveKey = KEY_NULL;
  };

  struct GamepadAxisBinding {
    int gamepad = 0;
    GamepadAxis axis = GAMEPAD_AXIS_LEFT_X;
    float deadzone = 0.15f;
  };

  struct Axis {
    std::vector<KeyAxisBinding> keyBindings;
    std::vector<GamepadAxisBinding> gamepadBindings;
  };

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
  std::unordered_map<std::string, Axis> m_Axes;
};
} // namespace Levye