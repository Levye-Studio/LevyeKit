#include "DesktopExporter.hpp"

#include <iostream>
#if defined(__APPLE__) || defined(__linux__) || defined(_WIN32)
#include <Levye/Platform/ExecutablePath.hpp>
#include <Levye/Project/ProjectLoader.hpp>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <toml++/toml.hpp>

#include "../BuildCommand.hpp"
#include "../ProcessArguments.hpp"
#include "WindowsPackage.hpp"
#endif
namespace Levye {
bool DesktopExporter::Execute(const std::filesystem::path& projectDirectory,
                              std::string_view format) {
  if (!format.empty()) {
    std::cerr << "[Levye] Android formats require --android.\n";
    return false;
  }
#if !defined(__APPLE__) && !defined(__linux__) && !defined(_WIN32)
  std::cerr
      << "[Levye] Desktop export is currently implemented only on macOS, Linux and Windows.\n";
  return false;
#else
  namespace fs = std::filesystem;
  try {
    const auto root = fs::canonical(projectDirectory);
    const auto project = ProjectLoader::Load(root / "levye.project");

    if (!project || !std::regex_match(project->target, std::regex("[A-Za-z_][A-Za-z0-9_]*")))
      return false;

    const auto appName = project->desktopAppName.empty() ? project->name : project->desktopAppName;

    auto bundleId = project->desktopBundleId;

    if (bundleId.empty()) {
      bundleId = "studio.levye." + project->target;
      for (auto& c : bundleId)
        if (c == '_')
          c = '-';
    }

    if (!std::regex_match(bundleId, std::regex("[A-Za-z0-9-]+(\\.[A-Za-z0-9-]+)+")) ||
        !std::regex_match(project->desktopVersionName, std::regex("[0-9]+(\\.[0-9]+){0,2}"))) {
      std::cerr << "[Levye] Invalid desktop bundle_id or version_name.\n";
      return false;
    }

    auto manifest = toml::parse_file((root / "levye.project").string());

    if (manifest["project"]["template_version"].value_or(0) < 2) {
      std::cerr << "[Levye] Run levye upgrade before desktop export.\n";
      return false;
    }

    const auto lock = root / ".levye-desktop-export.lock";

    if (!fs::create_directory(lock)) {
      std::cerr << "[Levye] Desktop export lock already exists.\n";
      return false;
    }

    struct Cleanup {
      fs::path path;
      ~Cleanup() {
        std::error_code ec;
        fs::remove_all(path, ec);
      }
    } cleanup{lock};

#ifdef __linux__
    if (std::system("command -v patchelf >/dev/null 2>&1 && command -v readelf >/dev/null 2>&1") !=
        0) {
      std::cerr << "[Levye] Linux export requires patchelf and readelf.\n";
      return false;
    }
#endif
    if (!BuildCommand::Execute(root, BuildConfiguration::Release, ProjectPlatform::Desktop))
      return false;

    if (auto* android = manifest["android"].as_table())
      android->erase("signing");

    const auto sanitized = lock / "levye.project";

    {
      std::ofstream out(sanitized);
      out << manifest;
      if (!out)
        return false;
    }
#ifdef _WIN32
    return WindowsPackage::Execute(root, *project, sanitized, appName, bundleId);
#else
    const auto script = ExecutablePath::GetDirectory().parent_path().parent_path().parent_path() /
#ifdef __APPLE__
                        "Platforms/Desktop/package-macos.sh";
#else
                        "Platforms/Desktop/package-linux.sh";
#endif
    std::string command = "bash " + LevyeCLI::QuoteArgument(script.string());
    for (const auto& arg : {root.string(), project->target, project->assetDirectory,
                            sanitized.string(), appName, bundleId, project->desktopVersionName,
                            std::to_string(project->desktopVersionCode), project->desktopIcon})
      command += " " + LevyeCLI::QuoteArgument(arg);
    return std::system(command.c_str()) == 0;
#endif
  } catch (const std::exception& error) {
    std::cerr << "[Levye] Desktop export failed: " << error.what() << '\n';
    return false;
  }
#endif
}
}  // namespace Levye
