#pragma once

#include <filesystem>
#include <string>

namespace Levye {

/**
 * @brief Generates new LevyeKit game projects from the default project
 * template.
 */
class ProjectGenerator {
public:
  /**
   * @brief Creates a new LevyeKit project.
   *
   * The project is generated inside the supplied output directory using the
   * framework's default project template.
   *
   * @param name Name of the project to create.
   * @param outputDirectory Directory in which the project folder is created.
   * @param templateDirectory Directory containing the default project
   *        template.
   * @return True when the project was generated successfully.
   */
  static bool Generate(const std::string &name,
                       const std::filesystem::path &outputDirectory,
                       const std::filesystem::path &templateDirectory);

private:
private:
  /**
   * @brief Replaces project-template tokens inside a text file.
   *
   * @param path Text file whose template tokens should be replaced.
   * @param projectName Name of the generated project.
   * @return True when the file was processed successfully.
   */
  static bool ProcessTemplateFile(const std::filesystem::path &path,
                                  const std::string &projectName);
};

} // namespace Levye