#pragma once

#include <cstdint>

namespace Levye {

/**
 * @brief Identifies an optional host-provided module.
 *
 * Values must remain stable once published.
 */
enum class ModuleID : std::uint32_t { Serialization = 1, ImGui = 2 };

/**
 * @brief Identifies the version of a module's function table.
 *
 * Each optional module maintains its own API version.
 */
using ModuleAPIVersion = std::uint32_t;

}  // namespace Levye