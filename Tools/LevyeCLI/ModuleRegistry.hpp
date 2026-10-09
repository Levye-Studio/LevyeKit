#pragma once

#include <string_view>

namespace Levye {

/**
 * @brief Registry of optional modules supported by the Levye CLI.
 */
class ModuleRegistry {
 public:
  /**
   * @brief Checks whether an optional module is supported.
   *
   * @param name Module identifier.
   * @return True when the module is registered.
   */
  static constexpr bool Exists(std::string_view name) {
    return name == "serialization" || name == "imgui";
  }
};

}  // namespace Levye