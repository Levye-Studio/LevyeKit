#include "RuntimePaths.hpp"

#include "SharedLibrary.hpp"

namespace Levye {
std::optional<RuntimePaths> RuntimePaths::Resolve(
    const std::filesystem::path& executableDirectory) {
  namespace fs = std::filesystem;

  if (executableDirectory.empty() || !executableDirectory.is_absolute())
    return std::nullopt;

  const auto parent = executableDirectory.parent_path();
  const auto module = SharedLibrary::MakeFilename("Game");

  RuntimePaths paths;

  if (executableDirectory.filename() == "MacOS" && parent.filename() == "Contents") {
    paths = {parent / "Resources", parent / "Frameworks" / module, true};
  } else if (executableDirectory.filename() == "bin" &&
             parent.parent_path().filename() == "Targets") {
    paths = {parent.parent_path().parent_path(), parent / "lib" / module, false};
  } else {
    // Portable desktop layout: bin/, lib/, levye.project and assets.
    paths = {parent, parent / "lib" / module, true};
  }

  std::error_code error;

  if (!fs::is_regular_file(paths.projectDirectory / "levye.project", error) || error ||
      !fs::is_regular_file(paths.modulePath, error) || error)
    return std::nullopt;

  return paths;
}
}  // namespace Levye
