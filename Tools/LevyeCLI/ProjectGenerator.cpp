#include "ProjectGenerator.hpp"

#include <fstream>
#include <iostream>
#include <iterator>

namespace Levye {

bool ProjectGenerator::Generate(
    const std::string &name, const std::filesystem::path &outputDirectory,
    const std::filesystem::path &templateDirectory) {
  if (name.empty()) {
    std::cerr << "[Levye] Project name cannot be empty.\n";

    return false;
  }

  if (!std::filesystem::exists(templateDirectory)) {
    std::cerr << "[Levye] Project template does not exist: "
              << templateDirectory << '\n';

    return false;
  }

  const std::filesystem::path projectDirectory = outputDirectory / name;

  if (std::filesystem::exists(projectDirectory)) {
    std::cerr << "[Levye] Directory already exists: " << projectDirectory
              << '\n';

    return false;
  }

  std::error_code error;

  std::filesystem::create_directories(projectDirectory, error);

  if (error) {
    std::cerr << "[Levye] Failed to create project directory: "
              << error.message() << '\n';

    return false;
  }

  std::filesystem::copy(templateDirectory, projectDirectory,
                        std::filesystem::copy_options::recursive, error);

  if (error) {
    std::cerr << "[Levye] Failed to copy project template: " << error.message()
              << '\n';

    std::filesystem::remove_all(projectDirectory);

    return false;
  }

  std::cout << "[Levye] Created project \"" << name << "\" at "
            << projectDirectory << '\n';

  const std::filesystem::path filesToProcess[] = {
      projectDirectory / "levye.project", projectDirectory / "CMakeLists.txt"};

  for (const auto &path : filesToProcess) {
    if (!ProcessTemplateFile(path, name)) {
      std::cerr << "[Levye] Failed to process template file: " << path << '\n';

      std::filesystem::remove_all(projectDirectory);

      return false;
    }
  }

  return true;
}

bool ProjectGenerator::ProcessTemplateFile(const std::filesystem::path &path,
                                           const std::string &projectName) {
  std::ifstream input(path);

  if (!input)
    return false;

  std::string content{std::istreambuf_iterator<char>(input),
                      std::istreambuf_iterator<char>()};

  input.close();

  constexpr const char *token = "{{PROJECT_NAME}}";

  std::size_t position = 0;

  while ((position = content.find(token, position)) != std::string::npos) {
    content.replace(position, std::char_traits<char>::length(token),
                    projectName);

    position += projectName.length();
  }

  std::ofstream output(path, std::ios::trunc);

  if (!output)
    return false;

  output << content;

  return output.good();
}

} // namespace Levye