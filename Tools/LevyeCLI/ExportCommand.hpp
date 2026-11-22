#pragma once

#include <filesystem>
#include <string>

#include "BuildConfiguration.hpp"

namespace Levye {

/** @brief Platform and output-format selection for project export. */
struct ExportOptions {
  ProjectPlatform platform = ProjectPlatform::ProjectDefault;
  std::string format;
};

/** @brief Dispatches project export to the selected platform packager. */
class ExportCommand {
 public:
  /** @brief Exports a project using its default or explicitly selected platform. */
  static bool Execute(const std::filesystem::path& projectDirectory, const ExportOptions& options);
};

}  // namespace Levye
