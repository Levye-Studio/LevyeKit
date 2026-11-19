#include "ExportCommand.hpp"

#include <iostream>

#include "Export/AndroidExporter.hpp"

namespace Levye {

bool ExportCommand::Execute(const std::filesystem::path& projectDirectory,
                            const ExportOptions& options) {
  switch (options.platform) {
    case ProjectPlatform::Android:
      return AndroidExporter::Execute(projectDirectory, options.format);

    default:
      std::cerr << "[Levye] Export is unavailable for "
                   "the selected platform.\n";

      return false;
  }
}

}  // namespace Levye