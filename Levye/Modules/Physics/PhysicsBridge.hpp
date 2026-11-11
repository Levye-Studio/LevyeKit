#pragma once

#include <Levye/Modules/Physics/PhysicsAPI.hpp>

namespace Levye {

class PhysicsService;

/**
 * @brief Creates a versioned API table for the host-owned physics service.
 *
 * The returned table contains pointers to host functions, never to
 * functions inside the reloadable game library.
 */
[[nodiscard]] PhysicsAPI MakePhysicsAPI();

}  // namespace Levye
