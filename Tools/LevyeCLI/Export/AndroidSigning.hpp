#pragma once

#include <filesystem>

namespace Levye {

class AndroidSigning {
 public:
  static bool EnsureConfigured(const std::filesystem::path& projectDirectory);
};

}  // namespace Levye