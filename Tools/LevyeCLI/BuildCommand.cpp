#include "BuildCommand.hpp"

#include <Levye/Platform/ExecutablePath.hpp>
#include <Levye/Project/ProjectLoader.hpp>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "FileTransaction.hpp"
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

bool BuildCommand::Execute(const std::filesystem::path& projectDirectory,
                           BuildConfiguration configuration, ProjectPlatform platform,
                           bool productionSign) {
  const std::string configurationName{ToString(configuration)};

  const std::filesystem::path buildDirectory = projectDirectory / "Build" / configurationName;
  const std::filesystem::path frameworkRoot =
      ExecutablePath::GetDirectory().parent_path().parent_path().parent_path();

  {
    auto project = ProjectLoader::Load(projectDirectory / "levye.project");
    if (!project) return false;
    platform = ResolveProjectPlatform(platform, project->defaultPlatform);
    if (productionSign &&
        (platform != ProjectPlatform::Android || configuration != BuildConfiguration::Release)) {
      std::cerr << "[Levye] --sign requires an Android Release build.\n";
      return false;
    }
    if (platform == ProjectPlatform::Android) {
      std::filesystem::path signingKeystore;
      if (productionSign) {
        if (project->androidSigningKeystore.empty() || project->androidSigningAlias.empty()) {
          std::cerr << "[Levye] --sign requires [android.signing] keystore and alias.\n";
          return false;
        }
        signingKeystore = project->androidSigningKeystore;
        if (signingKeystore.is_relative()) signingKeystore = projectDirectory / signingKeystore;
        std::error_code pathError;
        signingKeystore = std::filesystem::canonical(signingKeystore, pathError);
        if (pathError || !std::filesystem::is_regular_file(signingKeystore)) {
          std::cerr << "[Levye] Android signing keystore does not exist: "
                    << project->androidSigningKeystore << '\n';
          return false;
        }
      }
      const auto readFile = [](const std::filesystem::path& path) -> std::string {
        std::ifstream input(path, std::ios::binary);
        if (!input) return {};
        return {std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
      };
      std::vector<PendingFile> pending;
      const auto entry = projectDirectory / "Source/AndroidMain.cpp";
      if (!std::filesystem::exists(entry)) {
        const auto templateEntry = frameworkRoot / "Templates/Default/Source/AndroidMain.cpp";
        auto content = readFile(templateEntry);
        if (content.empty()) {
          std::cerr << "[Levye] Android entry template is missing: " << templateEntry << '\n';
          return false;
        }
        pending.push_back({entry, std::move(content), false});
      }
      const auto manifestPath = projectDirectory / "levye.project";
      auto manifest = readFile(manifestPath);
      if (manifest.empty()) {
        std::cerr << "[Levye] Cannot read levye.project.\n";
        return false;
      }

      bool manifestChanged = false;

      if (!project->useLogicalDisplay) {
        const bool landscape = project->androidOrientation == "landscape";

        const int logicalWidth = landscape ? 1280 : 720;
        const int logicalHeight = landscape ? 720 : 1280;

        manifest +=
            "\n[display]\n"
            "logical_width = " +
            std::to_string(logicalWidth) + "\n" +
            "logical_height = " + std::to_string(logicalHeight) + "\n" + "scaling = \"fit\"\n";

        manifestChanged = true;
      }

      if (project->androidPackage.empty()) {
        if (manifest.find("[android]") != std::string::npos) {
          std::cerr << "[Levye] Set [android] package in levye.project.\n";
          return false;
        }

        std::string suffix;

        for (unsigned char character : project->target) {
          if (std::isalnum(character)) {
            suffix += static_cast<char>(std::tolower(character));
          }
        }

        if (suffix.empty()) return false;

        manifest +=
            "\n[android]\n"
            "package = \"com.levyekit.game" +
            suffix +
            "\"\n"
            "orientation = \"portrait\"\n";

        manifestChanged = true;
      }

      if (manifestChanged) {
        pending.push_back({manifestPath, std::move(manifest), true});
      }

      const auto cmakePath = projectDirectory / "CMakeLists.txt";
      auto cmake = readFile(cmakePath);
      if (cmake.empty()) {
        std::cerr << "[Levye] Cannot read project CMakeLists.txt.\n";
        return false;
      }
      if (cmake.find("Source/AndroidMain.cpp") == std::string::npos) {
        const auto templateCMake = readFile(frameworkRoot / "Templates/Default/CMakeLists.txt");
        const auto begin = templateCMake.find("if(LEVYE_WITH_ANDROID)\n    add_library(");
        const auto end =
            begin == std::string::npos ? std::string::npos : templateCMake.find("\nendif()", begin);
        const auto insertion = cmake.find("add_library(\n    Game");
        if (begin == std::string::npos || end == std::string::npos ||
            insertion == std::string::npos || cmake.find("Android SHARED") != std::string::npos) {
          std::cerr << "[Levye] Cannot safely add Android to this custom CMakeLists.txt.\n";
          return false;
        }
        cmake.insert(insertion, templateCMake.substr(begin, end + 8 - begin) + "\n\n");
        pending.push_back({cmakePath, std::move(cmake), true});
      }
      if (!pending.empty()) {
        std::string error;
        if (!CommitFiles(projectDirectory, pending, error)) {
          std::cerr << "[Levye] Failed to add Android configuration: " << error << '\n';
          return false;
        }
        project = ProjectLoader::Load(manifestPath);
        if (!project || project->androidPackage.empty()) return false;
      }
      const auto script = frameworkRoot / "Platforms/Android/package-project.sh";
      std::string command = "bash " + LevyeCLI::QuoteArgument(script.string());
      const auto addArgument = [&](const std::string& value) {
        command += " " + LevyeCLI::QuoteArgument(value);
      };
      addArgument(projectDirectory.string());
      addArgument(configurationName);
      addArgument(project->target);
      addArgument(project->androidPackage);
      addArgument(project->assetDirectory);
      addArgument(project->androidOrientation);
      addArgument(project->androidAppName);
      addArgument(project->androidVersionName);
      addArgument(std::to_string(project->androidVersionCode));
      addArgument(project->androidIcon);
      addArgument(project->androidAdaptiveIconForeground);
      addArgument(project->androidAdaptiveIconBackground);
      if (productionSign) {
        command += " --sign";
        addArgument(signingKeystore.string());
        addArgument(project->androidSigningAlias);
      }
      return std::system(command.c_str()) == 0;
    }
  }

  std::cout << "[Levye] Configuring project...\n";

  const std::string configureCommand =
      "cmake -S " + LevyeCLI::QuoteArgument(projectDirectory.string()) + " -B " +
      LevyeCLI::QuoteArgument(buildDirectory.string()) +
      " -DCMAKE_BUILD_TYPE=" + LevyeCLI::QuoteArgument(configurationName) +
      " -DCMAKE_EXPORT_COMPILE_COMMANDS=ON" +
      " -DLEVYE_ROOT=" + LevyeCLI::QuoteArgument(frameworkRoot.string());

  const int configureResult = std::system(configureCommand.c_str());

  if (configureResult != 0) {
    std::cerr << "[Levye] Project configuration failed.\n";

    return false;
  }

  std::cout << "[Levye] Building project...\n";

  // Select the configuration for multi-config generators on every platform.
  const std::string buildCommand = "cmake --build " +
                                   LevyeCLI::QuoteArgument(buildDirectory.string()) + " --config " +
                                   LevyeCLI::QuoteArgument(configurationName);

  const int buildResult = std::system(buildCommand.c_str());

  if (buildResult != 0) {
    std::cerr << "[Levye] Project build failed.\n";

    return false;
  }

  std::cout << "[Levye] Build successful.\n";

  return true;
}

}  // namespace Levye
