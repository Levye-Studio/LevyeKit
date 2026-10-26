#include "RunCommand.hpp"

#include <Levye/Project/ProjectLoader.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

#include "ProcessArguments.hpp"

namespace {

/**
 * @brief Wraps a filesystem path in quotes for use in a shell command.
 *
 * @param path Filesystem path to quote.
 * @return Quoted path string.
 */
std::string QuotePath(const std::filesystem::path& path) { return "\"" + path.string() + "\""; }

}  // namespace

namespace Levye {

bool RunCommand::Execute(const std::filesystem::path& projectDirectory,
                         BuildConfiguration configuration) {
  const std::filesystem::path projectFile = projectDirectory / "levye.project";

  const auto project = ProjectLoader::Load(projectFile);

  if (!project) {
    std::cerr << "[Levye] Failed to load project configuration.\n";

    return false;
  }

  std::filesystem::path executablePath =
      projectDirectory / "Targets" / std::string{ToString(configuration)} / "bin" / project->target;

#if defined(_WIN32)
  executablePath += ".exe";
#endif

  if (!std::filesystem::is_regular_file(executablePath)) {
    std::cerr << "[Levye] Game executable not found: " << executablePath << '\n'
              << "[Levye] Run 'levye build' first.\n";

    return false;
  }

  std::cout << "[Levye] Running " << project->target << "...\n";

  // cmd.exe strips the outer quotes of a command starting with a quoted path.
  const std::string command = LevyeCLI::QuoteArgument(executablePath.string());

  const int result = std::system(command.c_str());

  if (result != 0) {
    std::cerr << "[Levye] Game exited with an error.\n";

    return false;
  }

  return true;
}

}  // namespace Levye
