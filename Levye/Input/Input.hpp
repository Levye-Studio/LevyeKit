#pragma once

#include <raylib.h>

namespace Levye {
/**
 * @brief Provides centralized access to player input.
 *
 * Input exposes common keyboard, mouse and gamepad queries while keeping
 * raylib's native input types available to game code.
 *
 * These functions are defined inline so reloadable game modules can use
 * the input API without linking another copy of LevyeKit into the module.
 */
class Input {
public:
  // -----------------------------------------------------------------
  // Keyboard
  // -----------------------------------------------------------------

  /**
   * @brief Returns whether a keyboard key is currently held.
   *
   * @param key raylib keyboard key to query.
   */
  static bool IsKeyDown(KeyboardKey key) { return ::IsKeyDown(key); }

  /**
   * @brief Returns whether a keyboard key was pressed this frame.
   *
   * @param key raylib keyboard key to query.
   */
  static bool IsKeyPressed(KeyboardKey key) { return ::IsKeyPressed(key); }

  /**
   * @brief Returns whether a keyboard key was released this frame.
   *
   * @param key raylib keyboard key to query.
   */
  static bool IsKeyReleased(KeyboardKey key) { return ::IsKeyReleased(key); }

  // -----------------------------------------------------------------
  // Mouse
  // -----------------------------------------------------------------

  /**
   * @brief Returns whether a mouse button is currently held.
   *
   * @param button raylib mouse button to query.
   */
  static bool IsMouseButtonDown(MouseButton button) {
    return ::IsMouseButtonDown(button);
  }

  /**
   * @brief Returns whether a mouse button was pressed this frame.
   *
   * @param button raylib mouse button to query.
   */
  static bool IsMouseButtonPressed(MouseButton button) {
    return ::IsMouseButtonPressed(button);
  }

  /**
   * @brief Returns whether a mouse button was released this frame.
   *
   * @param button raylib mouse button to query.
   */
  static bool IsMouseButtonReleased(MouseButton button) {
    return ::IsMouseButtonReleased(button);
  }

  /**
   * @brief Returns the current mouse position in window coordinates.
   */
  static Vector2 GetMousePosition() { return ::GetMousePosition(); }

  /**
   * @brief Returns mouse movement since the previous frame.
   */
  static Vector2 GetMouseDelta() { return ::GetMouseDelta(); }

  /**
   * @brief Returns mouse wheel movement for the current frame.
   */
  static Vector2 GetMouseWheel() { return ::GetMouseWheelMoveV(); }

  // -----------------------------------------------------------------
  // Gamepad
  // -----------------------------------------------------------------

  /**
   * @brief Returns whether a gamepad is currently available.
   *
   * @param gamepad Gamepad index, starting at zero.
   */
  static bool IsGamepadAvailable(int gamepad) {
    return ::IsGamepadAvailable(gamepad);
  }

  /**
   * @brief Returns whether a gamepad button is currently held.
   *
   * @param gamepad Gamepad index.
   * @param button raylib gamepad button to query.
   */
  static bool IsGamepadButtonDown(int gamepad, GamepadButton button) {
    return ::IsGamepadButtonDown(gamepad, button);
  }

  /**
   * @brief Returns whether a gamepad button was pressed this frame.
   *
   * @param gamepad Gamepad index.
   * @param button raylib gamepad button to query.
   */
  static bool IsGamepadButtonPressed(int gamepad, GamepadButton button) {
    return ::IsGamepadButtonPressed(gamepad, button);
  }

  /**
   * @brief Returns whether a gamepad button was released this frame.
   *
   * @param gamepad Gamepad index.
   * @param button raylib gamepad button to query.
   */
  static bool IsGamepadButtonReleased(int gamepad, GamepadButton button) {
    return ::IsGamepadButtonReleased(gamepad, button);
  }

  /**
   * @brief Returns the current value of a gamepad axis.
   *
   * Axis values normally range from -1.0 to 1.0.
   *
   * @param gamepad Gamepad index.
   * @param axis raylib gamepad axis to query.
   */
  static float GetGamepadAxis(int gamepad, GamepadAxis axis) {
    return ::GetGamepadAxisMovement(gamepad, axis);
  }
};
} // namespace Levye