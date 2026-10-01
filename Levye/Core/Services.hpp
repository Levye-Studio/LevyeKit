#pragma once

#include "HostServices.hpp"

namespace Levye {

/**
 * @brief Provides access to the host services bound to the current game module.
 *
 * The service pointer is stored inside the reloadable game module rather than
 * the host. Public LevyeKit facades such as Input and Log use this class to
 * reach host-owned systems through the HostServices ABI.
 *
 * The host must bind its services before game callbacks use the public facade
 * API.
 */
class Services {
public:
  /**
   * @brief Binds host services to the current game module.
   *
   * @param services Services supplied by the LevyeKit host.
   *
   * @warning The supplied HostServices object must remain valid for the
   * lifetime of the loaded game module.
   */
  static void Bind(const HostServices *services) { s_Host = services; }

  /**
   * @brief Removes the current host-service binding.
   */
  static void Unbind() { s_Host = nullptr; }

  /**
   * @brief Returns the currently bound host services.
   *
   * @return Pointer to HostServices, or nullptr when no host is bound.
   */
  static const HostServices *Get() { return s_Host; }

  /**
   * @brief Returns whether host services are currently available.
   */
  static bool IsBound() { return s_Host != nullptr; }

private:
  /**
   * @brief Module-local pointer to the host service table.
   *
   * inline ensures there is one instance for the game module without
   * requiring a separately linked implementation file.
   */
  inline static const HostServices *s_Host = nullptr;
};

} // namespace Levye