#pragma once

#include <filesystem>
#include <optional>

namespace Levye {
/** @brief Locates project and game-module paths relative to the executable. */
struct RuntimePaths {
  std::filesystem::path projectDirectory;
  std::filesystem::path modulePath;
  bool distribution = false;

  /** @brief Resolves runtime files without depending on the working directory. */
  static std::optional<RuntimePaths> Resolve(const std::filesystem::path& executableDirectory);
};
}  // namespace Levye
