#pragma once

#include <filesystem>
#include <string>

#include "BuildConfiguration.hpp"

namespace Levye {

struct ExportOptions {
  ProjectPlatform platform = ProjectPlatform::ProjectDefault;
  std::string format;
};

class ExportCommand {
 public:
  static bool Execute(const std::filesystem::path& projectDirectory, const ExportOptions& options);
};

}  // namespace Levye