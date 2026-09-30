#include "ProjectGenerator.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

void PrintUsage() {
  std::cout << "LevyeKit CLI\n\n"
            << "Usage:\n"
            << "  levye new <ProjectName>\n";
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

    // Temporary while the CLI still lives inside the LevyeKit repository.
    const std::filesystem::path templateDirectory =
        std::filesystem::current_path() / "Templates/Default";

    if (!Levye::ProjectGenerator::Generate(projectName, outputDirectory,
                                           templateDirectory)) {
      return 1;
    }

    return 0;
  }

  std::cerr << "[Levye] Unknown command: " << command << "\n\n";

  PrintUsage();

  return 1;
}