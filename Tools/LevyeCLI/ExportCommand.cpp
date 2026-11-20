#include "ExportCommand.hpp"

#include <Levye/Project/ProjectLoader.hpp>
#include <iostream>

#include "Export/AndroidExporter.hpp"
#include "Export/DesktopExporter.hpp"

namespace Levye {

bool ExportCommand::Execute(const std::filesystem::path& projectDirectory,
                            const ExportOptions& options) {
  const auto project = ProjectLoader::Load(projectDirectory / "levye.project");

  if (!project) return false;

  const auto platform = ResolveProjectPlatform(options.platform, project->defaultPlatform);

  switch (platform) {
    case ProjectPlatform::Android:
      return AndroidExporter::Execute(projectDirectory, options.format);

    case ProjectPlatform::Desktop:
      return DesktopExporter::Execute(projectDirectory, options.format);

    default:
      std::cerr << "[Levye] Export is unavailable for "
                   "the selected platform.\n";

      return false;
  }
}

}  // namespace Levye