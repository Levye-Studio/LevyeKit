#include "RunCommand.hpp"

#include <Levye/Project/ProjectLoader.hpp>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#if defined(__APPLE__)
#include <sys/wait.h>
#endif

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

#if defined(__APPLE__)
bool CaptureCommand(const std::string& command, std::string& output) {
  FILE* pipe = popen((command + " 2>&1").c_str(), "r");
  if (!pipe) return false;
  std::array<char, 4096> buffer{};
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) output += buffer.data();
  const int status = pclose(pipe);
  return status != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
#endif

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

  if (platform == ProjectPlatform::IOS) {
#if !defined(__APPLE__)
    std::cerr << "[Levye] iOS simulator runs require macOS and Xcode.\n";
    return false;
#else
    if (!BuildCommand::Execute(projectDirectory, configuration, ProjectPlatform::IOS)) {
      std::cerr << "[Levye] iOS build failed before launch.\n";
      return false;
    }
    const auto app = projectDirectory / "Targets" /
                     ("ios-" + std::string{ToString(configuration)}) / "bin" /
                     (project->target + ".app");
    const auto info = app / "Info.plist";
    if (!std::filesystem::is_regular_file(info)) {
      std::cerr << "[Levye] iOS app not found: " << app
                << "\n[Levye] Run 'levye build --ios' first.\n";
      return false;
    }
    std::string identifier;
    if (!CaptureCommand(
            "plutil -extract CFBundleIdentifier raw -o - " + LevyeCLI::QuoteArgument(info.string()),
            identifier)) {
      std::cerr << "[Levye] Cannot read CFBundleIdentifier: " << identifier << '\n';
      return false;
    }
    while (!identifier.empty() && (identifier.back() == '\n' || identifier.back() == '\r'))
      identifier.pop_back();
    if (identifier.empty() ||
        identifier.find_first_not_of(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_") !=
            std::string::npos) {
      std::cerr << "[Levye] Invalid iOS bundle identifier in " << info << '\n';
      return false;
    }
    std::string devices;
    if (!CaptureCommand("xcrun simctl list devices available", devices)) {
      std::cerr << "[Levye] Cannot query iOS simulators: " << devices << '\n';
      return false;
    }
    std::string booted, available, name, availableName;
    bool iosRuntime = false;
    const std::regex devicePattern(R"(^\s*(.+?)\s+\(([0-9A-Fa-f-]{36})\)\s+\((Booted|Shutdown)\))");
    const std::regex runtimePattern(R"(^--\s*(.+?)\s*--$)");
    std::istringstream lines(devices);
    std::string line;
    std::smatch match;
    while (std::getline(lines, line)) {
      if (std::regex_search(line, match, runtimePattern)) {
        iosRuntime = match[1].str().rfind("iOS ", 0) == 0;
      } else if (iosRuntime && std::regex_search(line, match, devicePattern)) {
        if (match[3] == "Booted") {
          booted = match[2];
          name = match[1];
          break;
        }
        if (available.empty()) {
          available = match[2];
          availableName = match[1];
        }
      }
    }
    const std::string device = booted.empty() ? available : booted;
    if (device.empty()) {
      std::cerr
          << "[Levye] No available iOS simulator. Install an iOS simulator runtime in Xcode.\n";
      return false;
    }
    if (booted.empty()) {
      name = availableName;
      std::cout << "[Levye] Booting simulator " << name << "...\n" << std::flush;
      if (std::system(("xcrun simctl boot " + device).c_str()) != 0 ||
          std::system(("xcrun simctl bootstatus " + device + " -b").c_str()) != 0) {
        std::cerr << "[Levye] Could not boot iOS simulator " << device << ".\n";
        return false;
      }
    }
    std::cout << "[Levye] Installing " << app << " on " << name << " (" << device << ")...\n"
              << std::flush;
    if (std::system(("xcrun simctl install " + device + " " + LevyeCLI::QuoteArgument(app.string()))
                        .c_str()) != 0) {
      std::cerr << "[Levye] iOS simulator installation failed.\n";
      return false;
    }
    std::cout << "[Levye] Launching " << identifier << "...\n" << std::flush;
    if (std::system(("xcrun simctl launch " + device + " " + LevyeCLI::QuoteArgument(identifier))
                        .c_str()) != 0) {
      std::cerr << "[Levye] iOS simulator launch failed.\n";
      return false;
    }
    return true;
#endif
  }

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
