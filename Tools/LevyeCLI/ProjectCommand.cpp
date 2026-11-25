#include "ProjectCommand.hpp"

#include <Levye/Project/ProjectLoader.hpp>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <string>
#include <toml++/toml.hpp>

#include "FileTransaction.hpp"
#include "PlatformPreparation.hpp"

namespace Levye {
namespace {

bool UpdateDefault(std::string& content, std::string_view requested) {
  const std::regex section(R"((^|\n)[ \t]*\[platform\][ \t]*(?:#.*)?(?:\r?\n|$))");
  std::smatch match;
  if (!std::regex_search(content, match, section)) {
    if (!content.empty() && content.back() != '\n')
      content += '\n';
    content += "\n[platform]\ndefault = \"" + std::string(requested) + "\"\n";
    return true;
  }
  const auto start = static_cast<std::size_t>(match.position() + match.length());
  const std::regex nextTable(R"(^[ \t]*(?:\[\[[^\]\r\n]+\]\]|\[[^\]\r\n]+\])[ \t]*(?:#.*)?\r?$)");
  std::size_t end = content.size();
  for (std::size_t line = start; line < content.size();) {
    const auto lineEnd = content.find('\n', line);
    if (std::regex_match(content.substr(line, lineEnd - line), nextTable)) {
      end = line;
      break;
    }
    if (lineEnd == std::string::npos)
      break;
    line = lineEnd + 1;
  }
  std::string body = content.substr(start, end - start);
  const std::regex setting(R"lev((^|\n)[ \t]*default[ \t]*=[ \t]*"([^"]*)")lev");
  std::smatch value;
  if (std::regex_search(body, value, setting)) {
    body.replace(static_cast<std::size_t>(value.position(2)),
                 static_cast<std::size_t>(value.length(2)), requested);
  } else {
    if (!body.empty() && body.back() != '\n')
      body += '\n';
    body += "default = \"" + std::string(requested) + "\"\n";
  }
  content.replace(start, end - start, body);
  return true;
}

}  // namespace

bool ProjectCommand::Execute(const std::filesystem::path& projectDirectory,
                             std::string_view requestedDefault) {
  const auto path = projectDirectory / "levye.project";
  auto project = ProjectLoader::Load(path);
  if (!project)
    return false;
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    std::cerr << "[Levye] Cannot read levye.project.\n";
    return false;
  }
  std::string content{std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
  toml::table manifest;
  try {
    manifest = toml::parse(content);
  } catch (const toml::parse_error& error) {
    std::cerr << "[Levye] Invalid levye.project: " << error << '\n';
    return false;
  }

  bool platformSupported =
      std::filesystem::is_regular_file(projectDirectory / "Source/Game.cpp") &&
      std::filesystem::is_directory(projectDirectory / project->assetDirectory);
  for (const char* module : {"serialization", "imgui", "animation", "physics"})
    if (manifest["modules"][module].value_or(false))
      platformSupported = false;
  const bool androidSupported = platformSupported;
  const bool iosSupported = platformSupported;
  const bool webSupported = platformSupported;
  if (!requestedDefault.empty()) {
    if (requestedDefault != "desktop" && requestedDefault != "android" &&
        requestedDefault != "ios" && requestedDefault != "web") {
      std::cerr << "[Levye] Unsupported default platform: " << requestedDefault
                << ". Choose desktop, android, ios, or web.\n";
      return false;
    }
    if (requestedDefault == "android" && !androidSupported) {
      std::cerr << "[Levye] Android requires Source/Game.cpp, assets, and no optional modules.\n";
      return false;
    }
    if (requestedDefault == "ios" && !iosSupported) {
      std::cerr << "[Levye] iOS requires Source/Game.cpp, the configured asset directory, "
                   "and no optional modules.\n";
      return false;
    }
    if (requestedDefault == "web" && !webSupported) {
      std::cerr << "[Levye] Web requires Source/Game.cpp, the configured asset directory, "
                   "and no optional modules.\n";
      return false;
    }
    bool initialized = false;
    if (requestedDefault == "android" || requestedDefault == "ios") {
      if (!PreparePlatformManifest(
              projectDirectory,
              requestedDefault == "ios" ? ProjectPlatform::IOS : ProjectPlatform::Android, content,
              initialized))
        return false;
    }
    if (initialized || project->defaultPlatform != requestedDefault ||
        !manifest["platform"]["default"].node()) {
      UpdateDefault(content, requestedDefault);
      try {
        manifest = toml::parse(content);
      } catch (const toml::parse_error& error) {
        std::cerr << "[Levye] Updated manifest is invalid: " << error << '\n';
        return false;
      }
      std::string error;
      if (!CommitFiles(projectDirectory, {{path, content, true}}, error, nullptr,
                       [&path, requestedDefault](std::string& validationError) {
                         const auto updated = ProjectLoader::Load(path);
                         if (updated && updated->defaultPlatform == requestedDefault)
                           return true;
                         validationError = "Default platform validation failed.";
                         return false;
                       })) {
        std::cerr << "[Levye] Cannot update default platform: " << error << '\n';
        return false;
      }
      project = ProjectLoader::Load(path);
      if (!project)
        return false;
    }
  }

  std::cout << "[Levye] Project: " << project->name << '\n';
  if (const auto version = manifest["project"]["template_version"].value<int64_t>())
    std::cout << "[Levye] Template: " << *version << '\n';
  else
    std::cout << "[Levye] Template: legacy\n";
  std::cout << "[Levye] Default platform: "
            << (project->defaultPlatform == "android" ? "Android"
                : project->defaultPlatform == "ios"   ? "iOS"
                : project->defaultPlatform == "web"   ? "Web"
                                                      : "Desktop")
            << '\n'
            << "[Levye] Supported platforms: Desktop" << (androidSupported ? ", Android" : "")
            << (iosSupported ? ", iOS" : "") << (webSupported ? ", Web" : "") << '\n'
            << "[Levye] Configuration: Debug\n"
            << "[Levye] Root: " << projectDirectory.string() << '\n';
  return true;
}

}  // namespace Levye
