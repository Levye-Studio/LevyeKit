#pragma once

#include <Levye/Project/ProjectConfig.hpp>

#include <filesystem>
#include <optional>

namespace Levye {

/**
 * @brief Loads LevyeKit project configuration files.
 *
 * ProjectLoader reads a levye.project file and converts its contents into a
 * ProjectConfig used by the host application.
 */
class ProjectLoader {
public:
  /**
   * @brief Loads a project configuration from disk.
   *
   * Paths stored in the returned ProjectConfig remain relative to the
   * project directory unless explicitly documented otherwise.
   *
   * @param path Path to the levye.project file.
   * @return Loaded project configuration, or std::nullopt when the file
   *         cannot be read or contains invalid configuration.
   */
  static std::optional<ProjectConfig> Load(const std::filesystem::path &path);
};

} // namespace Levye