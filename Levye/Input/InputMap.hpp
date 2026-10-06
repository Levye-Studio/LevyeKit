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
   * @brief Host-side input sampling functions; defaults to raylib.
   *
   * Tests may supply hardware-free functions. All callbacks must be non-null
   * and remain valid for the lifetime of the map (never use module callbacks).
   */
  struct InputState {
    bool (*isKeyDown)(int) = ::IsKeyDown;
    bool (*isMouseButtonDown)(int) = ::IsMouseButtonDown;
    Vector2 (*getMouseDelta)() = ::GetMouseDelta;
    float (*getMouseWheelMove)() = ::GetMouseWheelMove;
    bool (*isGamepadAvailable)(int) = ::IsGamepadAvailable;
    bool (*isGamepadButtonDown)(int, int) = ::IsGamepadButtonDown;
    float (*getGamepadAxisMovement)(int, int) = ::GetGamepadAxisMovement;
  };

  InputMap() = default;
  explicit InputMap(InputState input) : m_Input(input) {}

  /**
   * @brief Samples actions and axes once per host frame.
   *
   * Call after raylib polls input and before reload/game callbacks. Queries
   * read this snapshot without consuming transitions. New bindings become
   * active at the next Update(); duplicate registrations preserve state.
   */
  void Update();

  /**
   * @brief Adds a keyboard binding to an action.
   *
   * The action is created automatically if it does not already exist.
   *
   * @param action Logical action name.
   * @param key Keyboard key associated with the action.
   */
  void BindKey(const std::string& action, KeyboardKey key);

  /**
   * @brief Adds a mouse button binding to an action.
   *
   * The action is created automatically if it does not already exist.
   * Registering the same button more than once is a no-op.
   *
   * @param action Logical action name.
   * @param button Mouse button associated with the action.
   */
  void BindMouseButton(const std::string& action, MouseButton button);

  /**
   * @brief Returns the mouse movement sampled during the current host frame.
   *
   * The value is the raw raylib mouse delta and remains unchanged for the
   * duration of the frame.
   *
   * @return Mouse movement since the previous host frame.
   */
  Vector2 GetMouseDelta() const;

  /**
   * @brief Returns the mouse wheel movement sampled during the current host
   * frame.
   *
   * Positive and negative values represent scrolling in opposite directions.
   * The value remains unchanged for the duration of the frame.
   *
   * @return Mouse wheel movement for the current frame.
   */
  float GetMouseWheel() const;

  /**
   * @brief Adds a gamepad button binding to an action.
   *
   * @param action Logical action name.
   * @param gamepad Gamepad index.
   * @param button Gamepad button associated with the action.
   */
  void BindGamepadButton(const std::string& action, int gamepad,
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
  void BindKeyAxis(const std::string& axis, KeyboardKey negativeKey,
                   KeyboardKey positiveKey);

  /**
   * @brief Adds a gamepad analog axis to a named logical axis.
   *
   * @param axis Logical axis name.
   * @param gamepad Gamepad index.
   * @param gamepadAxis Native raylib gamepad axis.
   * @param deadzone Magnitudes below this threshold are zero, without
   * rescaling. Clamped to [0, 1]; non-finite values use 0.15.
   */
  void BindGamepadAxis(const std::string& axis, int gamepad,
                       GamepadAxis gamepadAxis, float deadzone = 0.15f);

  /**
   * @brief Returns the current value of a named logical axis.
   *
   * All held negative keys combine into one negative direction, and all
   * positive keys into one positive direction. Opposing directions cancel.
   * The keyboard result competes with deadzoned gamepad values by magnitude;
   * keyboard wins ties, then the first registered gamepad wins ties.
   * Values are sampled by Update() and clamped to [-1, 1].
   *
   * @param axis Logical axis name.
   * @return Axis value in the range -1 to +1.
   */
  float GetAxis(const std::string& axis) const;

  /**
   * @brief Returns whether any binding for an action is currently held.
   *
   * @param action Logical action name.
   */
  bool IsDown(const std::string& action) const;

  /**
   * @brief Returns whether the combined action changed from up to down this
   * frame.
   *
   * @param action Logical action name.
   */
  bool IsPressed(const std::string& action) const;

  /**
   * @brief Returns whether the last held binding was released this frame.
   *
   * @param action Logical action name.
   */
  bool IsReleased(const std::string& action) const;

  /**
   * @brief Removes an action and its snapshot immediately, without a release.
   *
   * A same-named axis is unaffected. Rebinding starts a fresh action.
   *
   * @param action Logical action name.
   */
  void ClearAction(const std::string& action);

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
    float value = 0.0f;
  };

 private:
  struct GamepadButtonBinding {
    int gamepad = 0;
    GamepadButton button = GAMEPAD_BUTTON_UNKNOWN;
  };

  struct Action {
    std::vector<KeyboardKey> keys;
    std::vector<MouseButton> mouseButtons;
    std::vector<GamepadButtonBinding> gamepadButtons;

    bool down = false;
    bool pressed = false;
    bool released = false;
  };

  InputState m_Input;

  std::unordered_map<std::string, Action> m_Actions;
  std::unordered_map<std::string, Axis> m_Axes;
  Vector2 m_MouseDelta{};
  float m_MouseWheel = 0.0f;
};
}  // namespace Levye