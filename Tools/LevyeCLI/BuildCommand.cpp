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
#include "PlatformPreparation.hpp"
#include "ProcessArguments.hpp"
#include "WebBuild.hpp"

namespace {

/**
 * @brief Wraps a filesystem path in quotes for use in a shell command.
 *
 * @param path Filesystem path to quote.
 * @return Quoted path string.
 */
std::string QuotePath(const std::filesystem::path& path) {
  return "\"" + path.string() + "\"";
}

}  // namespace

namespace Levye {

bool BuildCommand::Execute(const std::filesystem::path& projectDirectory,
                           BuildConfiguration configuration, ProjectPlatform platform,
                           bool productionSign, std::string_view androidFormat) {
  const std::string configurationName{ToString(configuration)};

  const std::filesystem::path buildDirectory = projectDirectory / "Build" / configurationName;
  const std::filesystem::path frameworkRoot =
      ExecutablePath::GetDirectory().parent_path().parent_path().parent_path();

  {
    auto project = ProjectLoader::Load(projectDirectory / "levye.project");

    if (!project)
      return false;

    platform = ResolveProjectPlatform(platform, project->defaultPlatform);

    if (platform == ProjectPlatform::Desktop) {
      std::string prepared;
      bool changed = false;
      if (!PreparePlatformManifest(projectDirectory, platform, prepared, changed))
        return false;
      if (changed) {
        std::string error;
        if (!CommitFiles(projectDirectory, {{projectDirectory / "levye.project", prepared, true}},
                         error)) {
          std::cerr << "[Levye] Failed to initialize desktop configuration: " << error << '\n';
          return false;
        }
      }
    }

    if (androidFormat != "apk" && androidFormat != "aab" && androidFormat != "all") {
      std::cerr << "[Levye] Invalid Android package format.\n";
      return false;
    }

    if (androidFormat != "apk" &&
        (platform != ProjectPlatform::Android || configuration != BuildConfiguration::Release ||
         !productionSign)) {
      std::cerr << "[Levye] AAB and combined packaging require a signed Android Release build.\n";
      return false;
    }

    if (productionSign &&
        (platform != ProjectPlatform::Android || configuration != BuildConfiguration::Release)) {
      std::cerr << "[Levye] --sign requires an Android Release build.\n";
      return false;
    }

    if (platform == ProjectPlatform::Web)
      return WebBuild::Execute(projectDirectory, configuration);

    if (platform == ProjectPlatform::IOS) {
#if !defined(__APPLE__)
      std::cerr << "[Levye] iOS simulator builds require macOS and Xcode.\n";
      return false;
#else
      if (std::system("xcodebuild -version > /dev/null 2>&1") != 0 ||
          std::system("xcrun --sdk iphonesimulator --show-sdk-path > /dev/null 2>&1") != 0) {
        std::cerr << "[Levye] Xcode and an iPhoneSimulator SDK are required.\n";
        return false;
      }
      const auto manifestPath = projectDirectory / "levye.project";
      std::string iosManifest;
      bool iosChanged = false;
      if (!PreparePlatformManifest(projectDirectory, ProjectPlatform::IOS, iosManifest, iosChanged))
        return false;
      if (iosChanged) {
        std::string error;
        if (!CommitFiles(projectDirectory, {{manifestPath, iosManifest, true}}, error)) {
          std::cerr << "[Levye] Failed to initialize iOS configuration: " << error << '\n';
          return false;
        }
        std::cout << "[Levye] Added [ios] configuration to levye.project.\n";
        project = ProjectLoader::Load(manifestPath);
        if (!project)
          return false;
      }
      const auto iosBuild = projectDirectory / "Build" / ("ios-" + configurationName);
      const auto iosSource = frameworkRoot / "Platforms/iOS";
      const auto assets = projectDirectory / project->assetDirectory;
      std::string bundleId = project->iosBundleId;
      if (bundleId.empty()) {
        bundleId = "com.levyekit.game";
        for (unsigned char character : project->target)
          if (std::isalnum(character))
            bundleId += static_cast<char>(std::tolower(character));
      }
      if (!std::filesystem::is_regular_file(projectDirectory / "Source/Game.cpp") ||
          !std::filesystem::is_directory(assets)) {
        std::cerr << "[Levye] iOS requires Source/Game.cpp and the configured asset directory.\n";
        return false;
      }
      std::string configure =
          "cmake -S " + LevyeCLI::QuoteArgument(iosSource.string()) + " -B " +
          LevyeCLI::QuoteArgument(iosBuild.string()) +
          " -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator"
          " -DCMAKE_OSX_DEPLOYMENT_TARGET=" +
          LevyeCLI::QuoteArgument(project->iosDeploymentTarget) +
          " -DCMAKE_OSX_ARCHITECTURES=arm64"
          " -DLEVYE_ROOT=" +
          LevyeCLI::QuoteArgument(frameworkRoot.string()) +
          " -DLEVYE_PROJECT_ROOT=" + LevyeCLI::QuoteArgument(projectDirectory.string()) +
          " -DLEVYE_IOS_ASSET_DIRECTORY=" + LevyeCLI::QuoteArgument(project->assetDirectory) +
          " -DLEVYE_IOS_DISPLAY_NAME=" +
          LevyeCLI::QuoteArgument(project->iosDisplayName.empty() ? project->name
                                                                  : project->iosDisplayName) +
          " -DLEVYE_IOS_OUTPUT_NAME=" + LevyeCLI::QuoteArgument(project->target) +
          " -DLEVYE_IOS_BUNDLE_ID=" + LevyeCLI::QuoteArgument(bundleId) +
          " -DLEVYE_IOS_VERSION_NAME=" + LevyeCLI::QuoteArgument(project->iosVersionName) +
          " -DLEVYE_IOS_VERSION_CODE=" +
          LevyeCLI::QuoteArgument(std::to_string(project->iosVersionCode)) +
          " -DLEVYE_IOS_DEPLOYMENT_TARGET=" + LevyeCLI::QuoteArgument(project->iosDeploymentTarget);
      configure += " -DLEVYE_IOS_ORIENTATION=" +
                   LevyeCLI::QuoteArgument(
                       project->displayOrientation == "auto" ? "all" : project->displayOrientation);
      std::cout << "[Levye] Configuring iOS simulator project...\n" << std::flush;
      if (std::system(configure.c_str()) != 0) {
        std::cerr << "[Levye] iOS CMake configuration failed.\n";
        return false;
      }
      const std::string build = "cmake --build " + LevyeCLI::QuoteArgument(iosBuild.string()) +
                                " --config " + LevyeCLI::QuoteArgument(configurationName);
      std::cout << "[Levye] Building iOS simulator app...\n" << std::flush;
      if (std::system(build.c_str()) != 0) {
        std::cerr << "[Levye] iOS application build failed.\n";
        return false;
      }
      const auto app = projectDirectory / "Targets" / ("ios-" + configurationName) / "bin" /
                       (project->target + ".app");
      if (!std::filesystem::is_regular_file(app / "Info.plist")) {
        std::cerr << "[Levye] iOS app bundle was not produced: " << app << '\n';
        return false;
      }
      std::cout << "[Levye] iOS app: " << app << '\n';
      return true;
#endif
    }

    if (platform == ProjectPlatform::Android) {
      std::filesystem::path signingKeystore;
      if (productionSign) {
        if (project->androidSigningKeystore.empty() || project->androidSigningAlias.empty()) {
          std::cerr << "[Levye] --sign requires [android.signing] keystore and alias.\n";
          return false;
        }
        signingKeystore = project->androidSigningKeystore;
        if (signingKeystore.is_relative())
          signingKeystore = projectDirectory / signingKeystore;
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
        if (!input)
          return {};
        return {std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
      };

      std::vector<PendingFile> pending;
      // Android entry points are owned by LevyeKit, not generated game projects.
      // Existing projects are migrated below when their CMake uses the known entry path.
      const auto manifestPath = projectDirectory / "levye.project";
      auto manifest = readFile(manifestPath);
      if (manifest.empty()) {
        std::cerr << "[Levye] Cannot read levye.project.\n";
        return false;
      }

      bool manifestChanged = false;
      if (!PreparePlatformManifest(projectDirectory, ProjectPlatform::Android, manifest,
                                   manifestChanged))
        return false;
      if (manifestChanged) {
        pending.push_back({manifestPath, std::move(manifest), true});
      }

      const auto cmakePath = projectDirectory / "CMakeLists.txt";
      auto cmake = readFile(cmakePath);
      if (cmake.empty()) {
        std::cerr << "[Levye] Cannot read project CMakeLists.txt.\n";
        return false;
      }
      const std::string legacyEntry = "${CMAKE_CURRENT_SOURCE_DIR}/Source/AndroidMain.cpp";
      const std::string frameworkEntry = "${LEVYE_ROOT}/Platforms/Android/AndroidMain.cpp";
      if (cmake.find(legacyEntry) != std::string::npos) {
        cmake.replace(cmake.find(legacyEntry), legacyEntry.size(), frameworkEntry);
        pending.push_back({cmakePath, cmake, true});
      } else if (cmake.find(frameworkEntry) == std::string::npos) {
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
        if (!project || project->androidPackage.empty())
          return false;
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
      addArgument(project->displayOrientation);
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

      command += " --format";
      addArgument(std::string(androidFormat));

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
