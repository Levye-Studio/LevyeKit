#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Levye {

/**
 * @brief Generates new LevyeKit game projects from the default project
 * template.
 */
class ProjectGenerator {
 public:
  /**
   * @brief Creates a new LevyeKit project from the default template.
   *
   * Template files are copied into a new project directory and supported
   * template tokens are replaced with values describing the generated project.
   *
   * @param name Name of the project to create.
   * @param outputDirectory Directory in which the project folder is created.
   * @param templateDirectory Directory containing the project template.
   * @param frameworkDirectory Root directory of the LevyeKit checkout.
   * @return True when the complete project was generated successfully.
   */
  static bool Generate(const std::string& name,
                       const std::filesystem::path& outputDirectory,
                       const std::filesystem::path& templateDirectory,
                       const std::filesystem::path& frameworkDirectory,
                       const std::vector<std::string>& modules = {});

  /**
   * @brief Converts a human-readable project name into a build-safe target.
   *
   * Spaces and unsupported characters are removed while alphanumeric
   * characters are preserved.
   *
   * @param name Human-readable project name.
   * @return Build-safe project target, or an empty string when no valid
   *         characters remain.
   */
  static std::string CreateTargetName(const std::string& name);

 private:
  /**
   * @brief Replaces supported tokens inside a generated text file.
   *
   * @param path File containing project-template tokens.
   * @param projectName Name of the generated project.
   * @param frameworkDirectory Root directory of the LevyeKit checkout.
   * @return True when the file was processed successfully.
   */
  static bool ProcessTemplateFile(
      const std::filesystem::path& path, const std::string& projectName,
      const std::string& projectTarget,
      const std::filesystem::path& frameworkDirectory);
};

}  // namespace Levye