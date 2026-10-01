#pragma once

#include <Levye/Core/Services.hpp>

namespace Levye {

/**
 * @brief Provides access to the host-owned logical screen system.
 *
 * Screen allows reloadable game code to change and query the active logical
 * screen without directly accessing the host's ScreenManager.
 */
class Screen {
public:
  /**
   * @brief Changes the active logical screen.
   *
   * @param name Name of the screen to activate.
   */
  static void Set(const char *name) {
    const HostServices *host = Services::Get();

    if (!host || !host->SetScreen || !name) {
      return;
    }

    host->SetScreen(host->context, name);
  }

  /**
   * @brief Returns whether a logical screen is currently active.
   *
   * @param name Name of the screen to test.
   * @return true when the specified screen is active.
   */
  static bool Is(const char *name) {
    const HostServices *host = Services::Get();

    if (!host || !host->IsScreen || !name) {
      return false;
    }

    return host->IsScreen(host->context, name);
  }
};

} // namespace Levye