#pragma once

#include <filesystem>

namespace Levye {

/** @brief Prepares project configuration for Android production signing. */
class AndroidSigning {
 public:
  /** @brief Ensures the project has a usable production-signing configuration. */
  static bool EnsureConfigured(const std::filesystem::path& projectDirectory);
};

}  // namespace Levye
