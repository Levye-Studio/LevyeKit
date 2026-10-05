#pragma once

#include <Levye/Core/Services.hpp>

namespace Levye {

/**
 * @brief Provides access to the host-owned input system.
 *
 * Input queries named actions and axes configured through LevyeKit's
 * InputMap. The implementation is header-only so reloadable game modules
 * communicate with the host exclusively through HostServices.
 *
 * Actions and axes are sampled once per host frame before reload callbacks.
 * Reads do not consume edges: Update, FixedUpdate and Draw share the snapshot.
 * Handle one-shot actions in OnUpdate; queue commands for fixed simulation.
 * Bindings and held state survive hot reload. New bindings take effect on the
 * next host frame; registering the same binding again does not reset state.
 *
 * Raw keyboard, mouse, and gamepad input can still be queried directly
 * through raylib when an action mapping is not required.
 */
class Input {
 public:
  /**
   * @brief Returns whether a named action is currently active.
   *
   * @param action Name of the configured input action.
   * @return true while any binding for the action is active.
   */
  static bool IsDown(const char* action) {
    const HostServices* host = Services::Get();

    if (!host || !host->IsActionDown || !action) {
      return false;
    }

    return host->IsActionDown(host->context, action);
  }

  /**
   * @brief Returns whether a named action became active this frame.
   *
   * @param action Name of the configured input action.
   * @return true only on the frame the action becomes active.
   */
  static bool IsPressed(const char* action) {
    const HostServices* host = Services::Get();

    if (!host || !host->IsActionPressed || !action) {
      return false;
    }

    return host->IsActionPressed(host->context, action);
  }

  /**
   * @brief Returns whether a named action was released this frame.
   *
   * @param action Name of the configured input action.
   * @return true only on the frame the action is released.
   */
  static bool IsReleased(const char* action) {
    const HostServices* host = Services::Get();

    if (!host || !host->IsActionReleased || !action) {
      return false;
    }

    return host->IsActionReleased(host->context, action);
  }

  /**
   * @brief Returns the current value of a named input axis.
   *
   * Keyboard directions combine across all bindings; opposing directions
   * cancel. The greatest magnitude wins between that result and deadzoned
   * gamepad axes. Ties prefer keyboard, then the first registered gamepad.
   *
   * @param axis Name of the configured input axis.
   * @return Current axis value, or 0 when unavailable.
   */
  static float GetAxis(const char* axis) {
    const HostServices* host = Services::Get();

    if (!host || !host->GetAxis || !axis) {
      return 0.0f;
    }

    return host->GetAxis(host->context, axis);
  }

  /**
   * @brief Adds a keyboard key to a named input action.
   *
   * Multiple keys can be bound to the same action.
   *
   * @param action Name of the action.
   * @param key raylib keyboard key code.
   */
  static void BindKey(const char* action, int key) {
    const HostServices* host = Services::Get();

    if (!host || !host->BindKey || !action) return;

    host->BindKey(host->context, action, key);
  }

  /**
   * @brief Adds a pair of keyboard keys to a named axis.
   *
   * @param axis Name of the axis.
   * @param negativeKey Key representing the negative direction.
   * @param positiveKey Key representing the positive direction.
   */
  static void BindKeyAxis(const char* axis, int negativeKey, int positiveKey) {
    const HostServices* host = Services::Get();

    if (!host || !host->BindKeyAxis || !axis) return;

    host->BindKeyAxis(host->context, axis, negativeKey, positiveKey);
  }

  /**
   * @brief Adds a gamepad button to an action; duplicate bindings are ignored.
   * @param action Logical action name.
   * @param gamepad Zero-based gamepad index.
   * @param button raylib gamepad button code.
   */
  static void BindGamepadButton(const char* action, int gamepad, int button) {
    const HostServices* host = Services::Get();
    if (!host || !host->BindGamepadButton || !action) return;
    host->BindGamepadButton(host->context, action, gamepad, button);
  }

  /**
   * @brief Adds an analog axis; rebinding updates its deadzone.
   * @param axis Logical axis name.
   * @param gamepad Zero-based gamepad index.
   * @param gamepadAxis raylib gamepad axis code.
   * @param deadzone Magnitudes below this threshold are zero, without
   * rescaling. Clamped to [0, 1]; non-finite values use 0.15.
   */
  static void BindGamepadAxis(const char* axis, int gamepad, int gamepadAxis,
                              float deadzone = 0.15f) {
    const HostServices* host = Services::Get();
    if (!host || !host->BindGamepadAxis || !axis) return;
    host->BindGamepadAxis(host->context, axis, gamepad, gamepadAxis, deadzone);
  }

  /**
   * @brief Removes an action and its state immediately, without a release
   * event.
   * @param action Logical action name. Same-named axes remain bound.
   */
  static void ClearAction(const char* action) {
    const HostServices* host = Services::Get();
    if (!host || !host->ClearAction || !action) return;
    host->ClearAction(host->context, action);
  }

  /**
   * @brief Removes all input actions, axes and snapshots immediately.
   *
   * This is explicit cleanup, not required during hot reload. Cleared actions
   * do not emit release events. Bindings must be registered again to use them.
   */
  static void Clear() {
    const HostServices* host = Services::Get();
    if (!host || !host->ClearInput) return;
    host->ClearInput(host->context);
  }
};

}  // namespace Levye