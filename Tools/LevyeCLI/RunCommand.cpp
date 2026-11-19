#include "RunCommand.hpp"

#include <Levye/Project/ProjectLoader.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

#include "BuildCommand.hpp"
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
                         BuildConfiguration configuration, ProjectPlatform platform) {
  const std::filesystem::path projectFile = projectDirectory / "levye.project";

  // Load the original project to determine its target and default platform.
  const auto project = ProjectLoader::Load(projectFile);

  if (!project) {
    std::cerr << "[Levye] Failed to load levye.project.\n";
    return false;
  }

  platform = ResolveProjectPlatform(platform, project->defaultPlatform);

  if (platform == ProjectPlatform::Android) {
#if defined(_WIN32)
    const int adbAvailable = std::system("adb version > NUL 2>&1");
#else
    const int adbAvailable = std::system("adb version > /dev/null 2>&1");
#endif

    if (adbAvailable != 0) {
      std::cerr << "[Levye] adb is unavailable. Install Android SDK "
                   "platform-tools and add adb to PATH.\n";
      return false;
    }

    // BuildCommand prepares Android support if the project was
    // originally created for desktop.
    if (std::filesystem::is_regular_file(projectDirectory / "CMakeLists.txt") &&
        !BuildCommand::Execute(projectDirectory, configuration, ProjectPlatform::Android)) {
      return false;
    }

    // Reload because BuildCommand may have updated levye.project.
    const auto androidProject = ProjectLoader::Load(projectFile);

    if (!androidProject || androidProject->androidPackage.empty() ||
        androidProject->androidPackage.find_first_not_of(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._") !=
            std::string::npos) {
      std::cerr << "[Levye] Invalid Android project configuration.\n";
      return false;
    }

    const auto apk = projectDirectory / "Targets" /
                     ("android-" + std::string{ToString(configuration)}) / "bin" /
                     (androidProject->target + ".apk");

    if (!std::filesystem::is_regular_file(apk)) {
      std::cerr << "[Levye] Android APK not found: " << apk << '\n';
      return false;
    }

    const std::string install = "adb install -r " + LevyeCLI::QuoteArgument(apk.string());

    if (std::system(install.c_str()) != 0) {
      std::cerr << "[Levye] Android install failed. Check adb devices.\n";
      return false;
    }

    const std::string launch =
        "adb shell am start -n " +
        LevyeCLI::QuoteArgument(androidProject->androidPackage + "/android.app.NativeActivity");

    return std::system(launch.c_str()) == 0;
  }

  // Desktop execution remains unchanged.
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

  const std::string command = LevyeCLI::QuoteArgument(executablePath.string());

  const int result = std::system(command.c_str());

  if (result != 0) {
    std::cerr << "[Levye] Game exited with an error.\n";
    return false;
  }

  return true;
}

}  // namespace Levye
