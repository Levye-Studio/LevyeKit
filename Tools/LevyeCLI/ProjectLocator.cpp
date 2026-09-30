#include "ProjectLocator.hpp"

namespace Levye {

std::optional<std::filesystem::path>
ProjectLocator::Find(const std::filesystem::path &startDirectory) {
  std::error_code error;

  std::filesystem::path current =
      std::filesystem::absolute(startDirectory, error);

  if (error)
    return std::nullopt;

  /*
   * Walk toward the filesystem root looking for the marker file that
   * identifies a LevyeKit project.
   */
  while (true) {
    const std::filesystem::path projectFile = current / "levye.project";

    if (std::filesystem::is_regular_file(projectFile, error)) {
      return current;
    }

    error.clear();

    const std::filesystem::path parent = current.parent_path();

    /*
     * At the filesystem root, parent_path() no longer advances the
     * search. Stop here to avoid an infinite loop.
     */
    if (parent == current || parent.empty()) {
      break;
    }

    current = parent;
  }

  return std::nullopt;
}

} // namespace Levye