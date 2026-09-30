#pragma once

#include <filesystem>
#include <optional>

namespace Levye {

/**
 * @brief Locates LevyeKit projects from a filesystem location.
 *
 * A LevyeKit project is identified by the presence of a levye.project file.
 * The locator searches the supplied directory and then walks upward through
 * its parent directories until a project is found.
 */
class ProjectLocator {
public:
  /**
   * @brief Searches for the nearest LevyeKit project.
   *
   * The search begins at startDirectory and continues through each parent
   * directory until levye.project is found or the filesystem root is
   * reached.
   *
   * @param startDirectory Directory from which the search should begin.
   * @return Path to the directory containing levye.project, or std::nullopt
   *         when no project could be found.
   */
  static std::optional<std::filesystem::path>
  Find(const std::filesystem::path &startDirectory);
};

} // namespace Levye