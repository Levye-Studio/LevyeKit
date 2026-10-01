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
  static bool IsDown(const char *action) {
    const HostServices *host = Services::Get();

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
  static bool IsPressed(const char *action) {
    const HostServices *host = Services::Get();

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
  static bool IsReleased(const char *action) {
    const HostServices *host = Services::Get();

    if (!host || !host->IsActionReleased || !action) {
      return false;
    }

    return host->IsActionReleased(host->context, action);
  }

  /**
   * @brief Returns the current value of a named input axis.
   *
   * Digital bindings normally produce values between -1 and 1. Analog
   * bindings may produce intermediate values depending on the device.
   *
   * @param axis Name of the configured input axis.
   * @return Current axis value, or 0 when unavailable.
   */
  static float GetAxis(const char *axis) {
    const HostServices *host = Services::Get();

    if (!host || !host->GetAxis || !axis) {
      return 0.0f;
    }

    return host->GetAxis(host->context, axis);
  }
};

} // namespace Levye