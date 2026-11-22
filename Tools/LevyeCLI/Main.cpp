#include <Levye/Core/Version.hpp>
#include <Levye/Platform/ExecutablePath.hpp>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "AddCommand.hpp"
#include "BuildCommand.hpp"
#include "CleanCommand.hpp"
#include "ConvertCommand.hpp"
#include "ExportCommand.hpp"
#include "GenerateCommand.hpp"
#include "ModuleRegistry.hpp"
#include "ProjectCommand.hpp"
#include "ProjectGenerator.hpp"
#include "ProjectLocator.hpp"
#include "RunCommand.hpp"
#include "UpgradeCommand.hpp"

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
            << "  add <module>     Add an optional module to the current project\n"
            << "  build            Build the current project\n"
            << "  run              Run the current project\n"
            << "  export           Export a distributable project package\n"
            << "  clean            Remove generated build files\n"
            << "  project          Show information about the current project\n"
            << "  generate <type> <name>  Generate C++ files\n"
            << "  g <type> <name>         Alias for generate\n"
            << "  upgrade          Upgrade an existing LevyeKit project\n"
            << "  convert          Convert a supported CMake C++ project\n"
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
  std::cout << "Create a new LevyeKit project.\n"
            << "\n"
            << "Usage:\n"
            << "  levye new <name> [options]\n"
            << "\n"
            << "Arguments:\n"
            << "  <name>            Name of the project to create\n"
            << "  --android         Make Android the default and include its entry point\n"
            << "  --desktop         Make Desktop the default\n"
            << "  --ios             Make iOS the default and generate its configuration\n"
            << "\n"
            << "Options:\n"
            << "  --git             Initialize a Git repository\n"
            << "  --commit          Initialize Git and create an initial commit\n"
            << "  -h, --help        Show this help message\n"
            << "  --with <module>   Enable an optional module\n";
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
            << "  --desktop         Build the desktop target\n"
            << "  --android         Build an Android ARM64 APK\n"
            << "  --ios             Build an iOS arm64 simulator app\n"
            << "  --sign            Sign an Android Release APK with [android.signing]\n"
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
            << "  --desktop         Run the desktop executable\n"
            << "  --android         Install and launch the Android APK\n"
            << "  --ios             Install and launch the iOS simulator app\n"
            << "  -h, --help        Show this help message\n"
            << "\n"
            << "The Debug configuration is used by default.\n"
            << "Android and iOS applications are rebuilt before launch.\n";
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
            << "  --desktop         Clean Desktop output\n"
            << "  --android         Clean Android output\n"
            << "  --ios             Clean iOS output\n"
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
            << "  levye project [--set-default desktop|android|ios]\n"
            << "\n"
            << "Options:\n"
            << "  --set-default     Persist a configured default platform\n"
            << "  -h, --help        Show this help message\n";
}

/**
 * @brief Prints help for the generate command.
 */
void PrintGenerateHelp() {
  std::cout << R"(
LevyeKit CLI — Code Generator

Usage:
  levye generate <type> <name> [options]
  levye g <type> <name> [options]

Types:
  class       Generate a C++ class (.hpp and .cpp)
  source      Generate a C++ source file (.cpp)
  header      Generate a C++ header file (.hpp)
  folder      Create a directory under Source/

Options:
  --force     Overwrite existing generated files
  --no-cmake  Do not update CMake source registration
  -h, --help  Display this help message

Examples:
  levye generate class Player
  levye generate class Systems/Physics
  levye generate source Gameplay/Combat
  levye generate header Core/Constants
  levye generate folder Levels

  levye g class Player
  levye g class Player --force
  levye g source Gameplay/Combat --no-cmake

Generated files:
  All paths are relative to the project's Source/ directory.
  Generated .cpp files are registered in Source/CMakeLists.txt
  unless --no-cmake is specified.

Notes:
  Run this command from inside a LevyeKit project.
  Existing files are protected unless --force is specified.
)";
}

/**
 * @brief Prints help for the upgrade command.
 */
void PrintUpgradeHelp() {
  std::cout << "Upgrade an existing LevyeKit project to the current template version.\n"
            << "\n"
            << "Usage:\n"
            << "  levye upgrade [options]\n"
            << "\n"
            << "Options:\n"
            << "  --dry-run         Show planned changes without applying them\n"
            << "  --check           Validate that the project is current\n"
            << "  -h, --help        Show this help message\n";
}

/**
 * @brief Prints help for the convert command.
 */
void PrintConvertHelp() {
  std::cout << "Convert a supported CMake C++ project to LevyeKit.\n"
            << "\n"
            << "Usage:\n"
            << "  levye convert [path] [--dry-run|--check]\n"
            << "\n"
            << "Options:\n"
            << "  --dry-run         Show the complete plan without modifying files\n"
            << "  --check           Return 2 if conversion is applicable and pending\n"
            << "  -h, --help        Show this help message\n";
}

void PrintExportHelp() {
  std::cout << "Export a distributable LevyeKit project.\n"
            << "\n"
            << "Usage:\n"
            << "  levye export [--desktop]\n"
            << "  levye export --html [--zip]\n"
            << "  levye export [--android] [--apk | --aab | --all]\n"
            << "  levye export --ios\n"
            << "\n"
            << "Options:\n"
            << "  --desktop         Export for desktop\n"
            << "  --android         Export for Android\n"
            << "  --html            Export for browsers\n"
            << "  --ios             Export a portable iOS source project\n"
            << "  --zip             Also create a Web ZIP archive\n"
            << "  --apk             Export an APK\n"
            << "  --aab             Export a signed Android App Bundle\n"
            << "  --all             Export both signed APK and AAB\n"
            << "  -h, --help        Show this help message\n"
            << "\n"
            << "Android exports use Release configuration and production signing.\n"
            << "Without a platform option, export uses the project default.\n";
}

/**
 * @brief Checks whether a command argument requests help.
 *
 * @param argument Command-line argument to inspect.
 * @return true when the argument is -h or --help.
 */
bool IsHelpArgument(std::string_view argument) { return argument == "-h" || argument == "--help"; }

/**
 * @brief Checks whether a command-line argument matches a specific option.
 *
 * @param argument Argument to inspect.
 * @param option Expected option.
 * @return true when the argument matches the option.
 */
bool IsOption(std::string_view argument, std::string_view option) { return argument == option; }

/**
 * @brief Initializes a Git repository inside a generated project.
 *
 * @param projectDirectory Root directory of the generated project.
 * @return true when Git initialization succeeds.
 */
bool InitializeGit(const std::filesystem::path& projectDirectory) {
  const std::string command = "git -C \"" + projectDirectory.string() + "\" init";

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
bool CreateInitialCommit(const std::filesystem::path& projectDirectory) {
  const std::string directory = projectDirectory.string();

  const std::string addCommand = "git -C \"" + directory + "\" add .";

  if (std::system(addCommand.c_str()) != 0) {
    return false;
  }

  const std::string commitCommand = "git -C \"" + directory + "\" commit -m \"Initial commit\"";

  return std::system(commitCommand.c_str()) == 0;
}

std::optional<std::filesystem::path> FindCurrentProject() {
  const auto projectDirectory = Levye::ProjectLocator::Find(std::filesystem::current_path());

  if (!projectDirectory) {
    std::cerr << "[Levye] No LevyeKit project found.\n";
  }

  return projectDirectory;
}

}  // namespace

int main(int argc, char** argv) {
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
    bool android = false;
    bool desktop = false;
    bool ios = false;

    std::vector<std::string> modules;

    for (int i = 3; i < argc; ++i) {
      const std::string_view argument = argv[i];

      if (argument == "--git") {
        initializeGit = true;
      } else if (argument == "--android") {
        if (desktop || ios) {
          std::cerr << "[Levye] Choose one project platform.\n";
          return 1;
        }
        android = true;
      } else if (argument == "--desktop") {
        if (android || ios) {
          std::cerr << "[Levye] Choose one project platform.\n";
          return 1;
        }
        desktop = true;
      } else if (argument == "--ios") {
        if (android || desktop) {
          std::cerr << "[Levye] Choose one project platform.\n";
          return 1;
        }
        ios = true;
      } else if (argument == "--commit") {
        createInitialCommit = true;
        initializeGit = true;
      } else if (argument == "--with") {
        if (i + 1 >= argc) {
          std::cerr << "[Levye] Missing module name after --with.\n";
          return 1;
        }

        const std::string module = argv[++i];

        if (!Levye::ModuleRegistry::Exists(module)) {
          std::cerr << "[Levye] Unknown module: " << module << '\n';
          return 1;
        }

        modules.push_back(module);
      } else {
        std::cerr << "[Levye] Unknown new option: " << argument << "\n\n";

        PrintNewHelp();
        return 1;
      }
    }

    const std::filesystem::path outputDirectory = std::filesystem::current_path();

    const std::filesystem::path executableDirectory = Levye::ExecutablePath::GetDirectory();

    const std::filesystem::path targetDirectory = executableDirectory.parent_path();

    const std::filesystem::path frameworkRoot = targetDirectory.parent_path().parent_path();

    const std::filesystem::path templateDirectory = frameworkRoot / "Templates/Default";

    if ((android || ios) && !modules.empty()) {
      std::cerr << "[Levye] Mobile platforms do not yet support optional modules. "
                   "Create a desktop project or omit --with.\n";
      return 1;
    }

    if (!Levye::ProjectGenerator::Generate(projectName, outputDirectory, templateDirectory,
                                           frameworkRoot, modules, android, ios)) {
      return 1;
    }

    // The generator uses the sanitized target name for the new directory.
    const std::filesystem::path projectDirectory =
        outputDirectory / Levye::ProjectGenerator::CreateTargetName(projectName);

    if (initializeGit) {
      if (!InitializeGit(projectDirectory)) {
        std::cerr << "[Levye] Project created, but Git initialization failed.\n";

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
    if (argc == 3 && IsHelpArgument(argv[2])) {
      PrintProjectHelp();
      return 0;
    }

    if (argc != 2 && (argc != 4 || std::string_view(argv[2]) != "--set-default")) {
      std::cerr << "[Levye] Usage: levye project [--set-default desktop|android|ios].\n\n";

      PrintProjectHelp();
      return 1;
    }

    const auto projectDirectory = FindCurrentProject();

    if (!projectDirectory) {
      std::cerr << "[Levye] No LevyeKit project found.\n";

      return 1;
    }

    return Levye::ProjectCommand::Execute(*projectDirectory, argc == 4 ? argv[3] : "") ? 0 : 1;
  }

  else if (command == "build") {
    if (argc >= 3 && IsHelpArgument(argv[2])) {
      PrintBuildHelp();
      return 0;
    }

    const auto projectDirectory = FindCurrentProject();

    if (!projectDirectory) return 1;

    Levye::BuildConfiguration configuration = Levye::BuildConfiguration::Debug;
    Levye::ProjectPlatform platform = Levye::ProjectPlatform::ProjectDefault;
    bool productionSign = false;

    for (int index = 2; index < argc; ++index) {
      const std::string option = argv[index];

      if (option == "--release") {
        configuration = Levye::BuildConfiguration::Release;
      } else if (option == "--sign") {
        productionSign = true;
      } else if (option == "--desktop") {
        if (platform != Levye::ProjectPlatform::ProjectDefault &&
            platform != Levye::ProjectPlatform::Desktop) {
          std::cerr << "[Levye] Conflicting platform flags.\n";
          return 1;
        }
        platform = Levye::ProjectPlatform::Desktop;
      } else if (option == "--android") {
        if (platform != Levye::ProjectPlatform::ProjectDefault &&
            platform != Levye::ProjectPlatform::Android) {
          std::cerr << "[Levye] Conflicting platform flags.\n";
          return 1;
        }
        platform = Levye::ProjectPlatform::Android;
      } else if (option == "--ios") {
        if (platform != Levye::ProjectPlatform::ProjectDefault &&
            platform != Levye::ProjectPlatform::IOS) {
          std::cerr << "[Levye] Conflicting platform flags.\n";
          return 1;
        }
        platform = Levye::ProjectPlatform::IOS;
      } else {
        std::cerr << "[Levye] Unknown build option: " << option << '\n';
        PrintBuildHelp();
        return 1;
      }
    }

    if (!Levye::BuildCommand::Execute(*projectDirectory, configuration, platform, productionSign)) {
      return 1;
    }

    return 0;
  }

  else if (command == "export") {
    if (argc >= 3 && IsHelpArgument(argv[2])) {
      PrintExportHelp();
      return 0;
    }

    Levye::ExportOptions options;
    bool platformSpecified = false;

    for (int index = 2; index < argc; ++index) {
      const std::string_view option = argv[index];

      if (option == "--android" || option == "--desktop" || option == "--html" ||
          option == "--ios") {
        if (platformSpecified) {
          std::cerr << "[Levye] Choose only one export platform.\n";
          return 1;
        }

        platformSpecified = true;

        options.platform = option == "--ios"       ? Levye::ProjectPlatform::IOS
                           : option == "--html"    ? Levye::ProjectPlatform::Web
                           : option == "--android" ? Levye::ProjectPlatform::Android
                                                   : Levye::ProjectPlatform::Desktop;

      } else if (option == "--apk" || option == "--aab" || option == "--all" || option == "--zip") {
        if (!options.format.empty()) {
          std::cerr << "[Levye] Choose only one export format.\n";
          return 1;
        }

        options.format = std::string(option.substr(2));

      } else {
        std::cerr << "[Levye] Unknown export option: " << option << '\n';

        PrintExportHelp();
        return 1;
      }
    }

    const auto projectDirectory = FindCurrentProject();

    if (!projectDirectory) return 1;

    return Levye::ExportCommand::Execute(*projectDirectory, options) ? 0 : 1;
  }

  else if (command == "add") {
    if (argc == 3 && IsHelpArgument(argv[2])) {
      std::cout << "Usage:\n"
                << "  levye add <module>\n"
                << "\n"
                << "Available modules:\n"
                << "  animation\n"
                << "  imgui\n"
                << "  physics\n"
                << "  serialization\n";

      return 0;
    }

    if (argc != 3) {
      std::cerr << "[Levye] Usage: levye add <module>\n";
      return 1;
    }

    const auto projectDirectory = FindCurrentProject();

    if (!projectDirectory) return 1;

    return Levye::AddCommand::Execute(*projectDirectory, argv[2]) ? 0 : 1;
  }

  else if (command == "run") {
    if (argc >= 3 && IsHelpArgument(argv[2])) {
      PrintRunHelp();
      return 0;
    }

    const auto projectDirectory = FindCurrentProject();

    if (!projectDirectory) return 1;

    Levye::BuildConfiguration configuration = Levye::BuildConfiguration::Debug;
    Levye::ProjectPlatform platform = Levye::ProjectPlatform::ProjectDefault;

    for (int index = 2; index < argc; ++index) {
      const std::string option = argv[index];

      if (option == "--release") {
        configuration = Levye::BuildConfiguration::Release;
      } else if (option == "--desktop") {
        if (platform != Levye::ProjectPlatform::ProjectDefault &&
            platform != Levye::ProjectPlatform::Desktop) {
          std::cerr << "[Levye] Conflicting platform flags.\n";
          return 1;
        }
        platform = Levye::ProjectPlatform::Desktop;
      } else if (option == "--android") {
        if (platform != Levye::ProjectPlatform::ProjectDefault &&
            platform != Levye::ProjectPlatform::Android) {
          std::cerr << "[Levye] Conflicting platform flags.\n";
          return 1;
        }
        platform = Levye::ProjectPlatform::Android;
      } else if (option == "--ios") {
        if (platform != Levye::ProjectPlatform::ProjectDefault &&
            platform != Levye::ProjectPlatform::IOS) {
          std::cerr << "[Levye] Conflicting platform flags.\n";
          return 1;
        }
        platform = Levye::ProjectPlatform::IOS;
      } else {
        std::cerr << "[Levye] Unknown run option: " << option << '\n';
        PrintRunHelp();
        return 1;
      }
    }

    if (!Levye::RunCommand::Execute(*projectDirectory, configuration, platform)) {
      return 1;
    }

    return 0;
  }

  else if (command == "generate" || command == "g") {
    if (argc == 3 && IsHelpArgument(argv[2])) {
      PrintGenerateHelp();
      return 0;
    }

    if (argc < 4) {
      PrintGenerateHelp();
      return 1;
    }

    Levye::GenerateOptions options;

    for (int i = 4; i < argc; ++i) {
      const std::string_view flag(argv[i]);

      if (flag == "--force") {
        options.force = true;
      } else if (flag == "--no-cmake") {
        options.noCMake = true;
      } else {
        std::cerr << "[Levye] Unknown generate option: " << flag << '\n';
        return 1;
      }
    }

    const auto projectDirectory = FindCurrentProject();

    if (!projectDirectory) {
      return 1;
    }

    return Levye::GenerateCommand::Execute(*projectDirectory, argv[2], argv[3], options) ? 0 : 1;
  }

  else if (command == "upgrade") {
    if (argc == 3 && IsHelpArgument(argv[2])) {
      PrintUpgradeHelp();
      return 0;
    }

    if (argc > 3 || (argc == 3 && std::string_view(argv[2]) != "--dry-run" &&
                     std::string_view(argv[2]) != "--check")) {
      std::cerr << "[Levye] Usage: levye upgrade [--dry-run|--check]\n";
      return 1;
    }

    const bool dryRun = argc == 3 && std::string_view(argv[2]) == "--dry-run";
    const bool check = argc == 3 && std::string_view(argv[2]) == "--check";

    const auto projectDirectory = FindCurrentProject();
    if (!projectDirectory) return 1;

    const auto executableDirectory = Levye::ExecutablePath::GetDirectory();

    const auto frameworkRoot = executableDirectory.parent_path().parent_path().parent_path();

    const auto result =
        Levye::UpgradeCommand::Execute(*projectDirectory, frameworkRoot, dryRun, check);
    return static_cast<int>(result);
  }

  else if (command == "convert") {
    if (argc >= 3 && IsHelpArgument(argv[2])) {
      PrintConvertHelp();
      return 0;
    }

    std::filesystem::path projectDirectory = std::filesystem::current_path();

    bool dryRun = false;
    bool check = false;
    bool pathProvided = false;

    for (int i = 2; i < argc; ++i) {
      const std::string_view argument = argv[i];

      if (argument == "--dry-run") {
        if (dryRun) {
          std::cerr << "[Levye] Duplicate --dry-run option.\n";
          return 1;
        }

        dryRun = true;
      } else if (argument == "--check") {
        if (check) {
          std::cerr << "[Levye] Duplicate --check option.\n";
          return 1;
        }
        check = true;
      } else if (!argument.empty() && argument.front() == '-') {
        std::cerr << "[Levye] Unknown option: " << argument << '\n';
        return 1;
      } else {
        if (pathProvided) {
          std::cerr << "[Levye] Only one project path is allowed.\n";
          return 1;
        }

        projectDirectory = std::filesystem::path(argv[i]);
        pathProvided = true;
      }
    }

    if (dryRun && check) {
      std::cerr << "[Levye] Choose either --dry-run or --check.\n";
      return 1;
    }
    const auto frameworkRoot =
        Levye::ExecutablePath::GetDirectory().parent_path().parent_path().parent_path();
    return Levye::ConvertCommand::Execute(projectDirectory, frameworkRoot, dryRun, check);
  }

  else if (command == "clean") {
    if (argc >= 3 && IsHelpArgument(argv[2])) {
      PrintCleanHelp();
      return 0;
    }

    const auto projectDirectory = FindCurrentProject();

    if (!projectDirectory) return 1;
    Levye::BuildConfiguration configuration = Levye::BuildConfiguration::Debug;
    Levye::ProjectPlatform platform = Levye::ProjectPlatform::ProjectDefault;
    bool all = false;
    for (int index = 2; index < argc; ++index) {
      const std::string_view option = argv[index];
      if (option == "--release")
        configuration = Levye::BuildConfiguration::Release;
      else if (option == "--all")
        all = true;
      else if (option == "--desktop" || option == "--android" || option == "--ios") {
        const auto selected = option == "--desktop"   ? Levye::ProjectPlatform::Desktop
                              : option == "--android" ? Levye::ProjectPlatform::Android
                                                      : Levye::ProjectPlatform::IOS;
        if (platform != Levye::ProjectPlatform::ProjectDefault && platform != selected) {
          std::cerr << "[Levye] Conflicting clean platform flags.\n";
          return 1;
        }
        platform = selected;
      } else {
        std::cerr << "[Levye] Unknown clean option: " << option << '\n';
        return 1;
      }
    }
    if (all && (platform != Levye::ProjectPlatform::ProjectDefault ||
                configuration == Levye::BuildConfiguration::Release)) {
      std::cerr << "[Levye] --all cannot be combined with platform or configuration flags.\n";
      return 1;
    }
    return all ? (Levye::CleanCommand::ExecuteAll(*projectDirectory) ? 0 : 1)
               : (Levye::CleanCommand::Execute(*projectDirectory, configuration, platform) ? 0 : 1);
  } else {
    std::cerr << "[Levye] Unknown command: " << command << "\n\n";

    std::cerr << "Run 'levye --help' for available commands.\n";

    return 1;
  }

  return 1;
}
