
#pragma once

#include <string>

namespace Levye {

// Migrates the legacy module configuration in a CMake file.
// Returns false if an unsupported or ambiguous layout is found.
// 'changed' is true only when the CMake content was modified.
/** @brief Migrates legacy generated-project module CMake settings. */
class ProjectModuleMigration {
 public:
  /** @brief Plans updates to module CMake configuration in memory. */
  static bool Prepare(std::string& cmake, bool& changed, std::string& error);
};

}  // namespace Levye
