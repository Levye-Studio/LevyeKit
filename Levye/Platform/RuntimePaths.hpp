#pragma once

#include <filesystem>
#include <optional>

namespace Levye {
// Resolve paths relative to the executable, never the working directory.
struct RuntimePaths {
  std::filesystem::path projectDirectory;
  std::filesystem::path modulePath;
  bool distribution = false;

  static std::optional<RuntimePaths> Resolve(const std::filesystem::path& executableDirectory);
};
}  // namespace Levye
