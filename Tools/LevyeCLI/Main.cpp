#include "BuildCommand.hpp"
#include "CleanCommand.hpp"
#include "ProjectGenerator.hpp"
#include "ProjectLocator.hpp"
#include "RunCommand.hpp"
#include <Levye/Core/Version.hpp>

#include <Levye/Platform/ExecutablePath.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {

/**
 * @brief Prints the main Levye CLI help message.
 */
void PrintHelp() {
  std::cout << "LevyeKit CLI\n"
            << "\n"
            << "Usage:\n"
            << "  levye <command> [options]\n"
            << "\n"
            << "Commands:\n"
            << "  new <name>       Create a new LevyeKit project\n"
            << "  build            Build the current project\n"
            << "  run              Run the current project\n"
            << "  clean            Remove generated build files\n"
            << "  project          Show information about the current project\n"
            << "\n"
            << "Options:\n"
            << "  -h, --help       Show this help message\n"
            << "  -v, --version    Show the LevyeKit CLI version\n"
            << "\n"
            << "Run 'levye <command> --help' for command-specific help.\n";
}

/**
 * @brief Prints the current LevyeKit version.
 */
void PrintVersion() { std::cout << "LevyeKit " << Levye::VERSION << '\n'; }

/**
 * @brief Prints help for the new command.
 */
void PrintNewHelp() {
  std::cout
      << "Create a new LevyeKit project.\n"
      << "\n"
      << "Usage:\n"
      << "  levye new <name> [options]\n"
      << "\n"
      << "Arguments:\n"
      << "  <name>            Name of the project to create\n"
      << "\n"
      << "Options:\n"
      << "  --git             Initialize a Git repository\n"
      << "  --commit          Initialize Git and create an initial commit\n"
      << "  -h, --help        Show this help message\n";
}

/**
 * @brief Prints help for the build command.
 */
void PrintBuildHelp() {
  std::cout << "Build the current LevyeKit project.\n"
            << "\n"
            << "Usage:\n"
            << "  levye build [options]\n"
            << "\n"
            << "Options:\n"
            << "  --release         Build the Release configuration\n"
            << "  -h, --help        Show this help message\n"
            << "\n"
            << "The Debug configuration is used by default.\n";
}

/**
 * @brief Prints help for the run command.
 */
void PrintRunHelp() {
  std::cout << "Run the current LevyeKit project.\n"
            << "\n"
            << "Usage:\n"
            << "  levye run [options]\n"
            << "\n"
            << "Options:\n"
            << "  --release         Run the Release configuration\n"
            << "  -h, --help        Show this help message\n"
            << "\n"
            << "The Debug configuration is used by default.\n"
            << "The project must be built before it can be run.\n";
}

/**
 * @brief Prints help for the clean command.
 */
void PrintCleanHelp() {
  std::cout << "Remove generated LevyeKit project files.\n"
            << "\n"
            << "Usage:\n"
            << "  levye clean [options]\n"
            << "\n"
            << "Options:\n"
            << "  --release         Clean the Release configuration\n"
            << "  --all             Clean all configurations\n"
            << "  -h, --help        Show this help message\n"
            << "\n"
            << "The Debug configuration is cleaned by default.\n";
}

/**
 * @brief Prints help for the project command.
 */
void PrintProjectHelp() {
  std::cout << "Show information about the current LevyeKit project.\n"
            << "\n"
            << "Usage:\n"
            << "  levye project\n"
            << "\n"
            << "Options:\n"
            << "  -h, --help        Show this help message\n";
}

/**
 * @brief Checks whether a command argument requests help.
 *
 * @param argument Command-line argument to inspect.
 * @return true when the argument is -h or --help.
 */
bool IsHelpArgument(std::string_view argument) {
  return argument == "-h" || argument == "--help";
}

/**
 * @brief Checks whether a command-line argument matches a specific option.
 *
 * @param argument Argument to inspect.
 * @param option Expected option.
 * @return true when the argument matches the option.
 */
bool IsOption(std::string_view argument, std::string_view option) {
  return argument == option;
}

/**
 * @brief Initializes a Git repository inside a generated project.
 *
 * @param projectDirectory Root directory of the generated project.
 * @return true when Git initialization succeeds.
 */
bool InitializeGit(const std::filesystem::path &projectDirectory) {
  const std::string command =
      "git -C \"" + projectDirectory.string() + "\" init";

  return std::system(command.c_str()) == 0;
}

/**
 * @brief Creates the initial Git commit for a generated project.
 *
 * All generated project files are staged before the commit is created.
 *
 * @param projectDirectory Root directory of the generated project.
 * @return true when the initial commit succeeds.
 */
bool CreateInitialCommit(const std::filesystem::path &projectDirectory) {
  const std::string directory = projectDirectory.string();

  const std::string addCommand = "git -C \"" + directory + "\" add .";

  if (std::system(addCommand.c_str()) != 0) {
    return false;
  }

  const std::string commitCommand =
      "git -C \"" + directory + "\" commit -m \"Initial commit\"";

  return std::system(commitCommand.c_str()) == 0;
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
    PrintHelp();
    return 0;
  }

  const std::string command = argv[1];

  if (command == "--help" || command == "-h") {
    PrintHelp();
    return 0;
  }

  if (command == "--version" || command == "-v") {
    PrintVersion();
    return 0;
  }

  if (command == "new") {
    if (argc < 3) {
      std::cerr << "[Levye] Missing project name.\n\n";

      PrintNewHelp();
      return 1;
    }

    if (IsHelpArgument(argv[2])) {
      PrintNewHelp();
      return 0;
    }

    const std::string projectName = argv[2];

    bool initializeGit = false;
    bool createInitialCommit = false;

    for (int i = 3; i < argc; ++i) {
      const std::string_view argument = argv[i];

      if (argument == "--git") {
        initializeGit = true;
      } else if (argument == "--commit") {
        createInitialCommit = true;
        initializeGit = true;
      } else {
        std::cerr << "[Levye] Unknown new option: " << argument << "\n\n";

        PrintNewHelp();
        return 1;
      }
    }

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

    if (!Levye::ProjectGenerator::Generate(projectName, outputDirectory,
                                           templateDirectory, frameworkRoot)) {
      return 1;
    }

    // ProjectGenerator uses the sanitized target as the directory name.
    // Use the actual generated project path here if you already have it
    // available from the generator.
    const std::filesystem::path projectDirectory =
        outputDirectory /
        Levye::ProjectGenerator::CreateTargetName(projectName);

    if (initializeGit) {
      if (!InitializeGit(projectDirectory)) {
        std::cerr
            << "[Levye] Project created, but Git initialization failed.\n";

        return 1;
      }

      std::cout << "[Levye] Git repository initialized.\n";

      if (createInitialCommit) {
        if (!CreateInitialCommit(projectDirectory)) {
          std::cerr << "[Levye] Git repository initialized, "
                    << "but the initial commit failed.\n";

          return 1;
        }

        std::cout << "[Levye] Initial commit created.\n";
      }
    }

    return 0;
  } else if (command == "project") {
    if (argc >= 3 && IsHelpArgument(argv[2])) {
      PrintProjectHelp();
      return 0;
    }

    if (argc > 2) {
      std::cerr
          << "[Levye] The 'project' command does not accept arguments.\n\n";

      PrintProjectHelp();
      return 1;
    }

    const auto projectDirectory = FindCurrentProject();

    if (!projectDirectory) {
      std::cerr << "[Levye] No LevyeKit project found.\n";

      return 1;
    }

    std::cout << "[Levye] Project: " << *projectDirectory << '\n';

    return 0;
  }

  else if (command == "build") {
    if (argc >= 3 && IsHelpArgument(argv[2])) {
      PrintBuildHelp();
      return 0;
    }

    if (argc > 3) {
      std::cerr << "[Levye] Too many arguments for 'build'.\n\n";

      PrintBuildHelp();
      return 1;
    }

    const auto projectDirectory = FindCurrentProject();

    if (!projectDirectory)
      return 1;

    Levye::BuildConfiguration configuration = Levye::BuildConfiguration::Debug;

    if (argc >= 3) {
      const std::string option = argv[2];

      if (option == "--release") {
        configuration = Levye::BuildConfiguration::Release;
      } else {
        std::cerr << "[Levye] Unknown build option: " << option << '\n';
        PrintBuildHelp();
        return 1;
      }
    }

    if (!Levye::BuildCommand::Execute(*projectDirectory, configuration)) {
      return 1;
    }

    return 0;
  }

  else if (command == "run") {
    if (argc >= 3 && IsHelpArgument(argv[2])) {
      PrintRunHelp();
      return 0;
    }

    if (argc > 3) {
      std::cerr << "[Levye] Too many arguments for 'run'.\n\n";

      PrintRunHelp();
      return 1;
    }

    const auto projectDirectory = FindCurrentProject();

    if (!projectDirectory)
      return 1;

    Levye::BuildConfiguration configuration = Levye::BuildConfiguration::Debug;

    if (argc >= 3) {
      const std::string option = argv[2];

      if (option == "--release") {
        configuration = Levye::BuildConfiguration::Release;
      } else {
        std::cerr << "[Levye] Unknown run option: " << option << '\n';
        PrintRunHelp();
        return 1;
      }
    }

    if (!Levye::RunCommand::Execute(*projectDirectory, configuration)) {
      return 1;
    }

    return 0;
  }

  else if (command == "clean") {
    if (argc >= 3 && IsHelpArgument(argv[2])) {
      PrintCleanHelp();
      return 0;
    }

    if (argc > 3) {
      std::cerr << "[Levye] Too many arguments for 'clean'.\n\n";

      PrintCleanHelp();
      return 1;
    }
    const auto projectDirectory = FindCurrentProject();

    if (!projectDirectory)
      return 1;

    if (argc >= 3) {
      const std::string option = argv[2];

      if (option == "--release") {
        if (!Levye::CleanCommand::Execute(*projectDirectory,
                                          Levye::BuildConfiguration::Release)) {
          return 1;
        }

        return 0;
      }

      if (option == "--all") {
        if (!Levye::CleanCommand::ExecuteAll(*projectDirectory)) {
          return 1;
        }

        return 0;
      }

      std::cerr << "[Levye] Unknown clean option: " << option << '\n';
      PrintCleanHelp();
      return 1;
    }

    if (!Levye::CleanCommand::Execute(*projectDirectory,
                                      Levye::BuildConfiguration::Debug)) {
      return 1;
    }

    return 0;
  } else {
    std::cerr << "[Levye] Unknown command: " << command << "\n\n";

    std::cerr << "Run 'levye --help' for available commands.\n";

    return 1;
  }

  return 1;
}