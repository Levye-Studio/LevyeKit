#include "BuildCommand.hpp"
#include "ProjectGenerator.hpp"
#include "ProjectLocator.hpp"

#include <Levye/Platform/ExecutablePath.hpp>

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

namespace {

void PrintUsage() {
  std::cout << "LevyeKit CLI\n\n"
            << "Usage:\n"
            << "  levye new <ProjectName>\n"
            << "  levye build\n"
            << "  levye project\n";
}

std::optional<std::filesystem::path> FindCurrentProject() {
  const auto projectDirectory =
      Levye::ProjectLocator::Find(std::filesystem::current_path());

  if (!projectDirectory) {
    std::cerr << "[Levye] No LevyeKit project found.\n";
  }

  return projectDirectory;
}

} // namespace

int main(int argc, char **argv) {
  if (argc < 2) {
    PrintUsage();
    return 1;
  }

  const std::string command = argv[1];

  if (command == "new") {
    if (argc < 3) {
      std::cerr << "[Levye] Missing project name.\n\n";

      PrintUsage();

      return 1;
    }

    const std::string projectName = argv[2];

    const std::filesystem::path outputDirectory =
        std::filesystem::current_path();

    const std::filesystem::path executableDirectory =
        Levye::ExecutablePath::GetDirectory();

    const std::filesystem::path targetDirectory =
        executableDirectory.parent_path();

    const std::filesystem::path frameworkRoot =
        targetDirectory.parent_path().parent_path();

    const std::filesystem::path templateDirectory =
        frameworkRoot / "Templates/Default";
    ;

    if (!Levye::ProjectGenerator::Generate(projectName, outputDirectory,
                                           templateDirectory, frameworkRoot)) {
      return 1;
    }

    return 0;
  }
  if (command == "project") {
    const auto projectDirectory = FindCurrentProject();

    if (!projectDirectory) {
      std::cerr << "[Levye] No LevyeKit project found.\n";

      return 1;
    }

    std::cout << "[Levye] Project: " << *projectDirectory << '\n';

    return 0;
  }

  if (command == "build") {
    const auto projectDirectory = FindCurrentProject();

    if (!projectDirectory)
      return 1;

    if (!Levye::BuildCommand::Execute(*projectDirectory)) {
      return 1;
    }

    return 0;
  }

  std::cerr << "[Levye] Unknown command: " << command << "\n\n";

  PrintUsage();

  return 1;
}