#include "ProjectCommand.hpp"

#include <Levye/Project/ProjectLoader.hpp>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <string>
#include <toml++/toml.hpp>

#include "FileTransaction.hpp"

namespace Levye {
namespace {

bool AndroidConfigured(const ProjectConfig& project, const toml::table& manifest,
                       const std::filesystem::path& root) {
  static const std::regex packagePattern(R"([A-Za-z][A-Za-z0-9_]*(\.[A-Za-z][A-Za-z0-9_]*)+)");
  if (!std::regex_match(project.androidPackage, packagePattern)) return false;
  for (const char* module : {"serialization", "imgui", "animation", "physics"})
    if (manifest["modules"][module].value_or(false)) return false;
  std::ifstream cmake(root / "CMakeLists.txt", std::ios::binary);
  if (!cmake) return false;
  const std::string content{std::istreambuf_iterator<char>{cmake},
                            std::istreambuf_iterator<char>{}};
  return content.find("Source/AndroidMain.cpp") != std::string::npos &&
         content.find("Android SHARED") != std::string::npos;
}

bool UpdateDefault(std::string& content, std::string_view requested) {
  const std::regex section(R"((^|\n)[ \t]*\[platform\][ \t]*(?:#.*)?(?:\r?\n|$))");
  std::smatch match;
  if (!std::regex_search(content, match, section)) {
    if (!content.empty() && content.back() != '\n') content += '\n';
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
    if (lineEnd == std::string::npos) break;
    line = lineEnd + 1;
  }
  std::string body = content.substr(start, end - start);
  const std::regex setting(R"lev((^|\n)[ \t]*default[ \t]*=[ \t]*"([^"]*)")lev");
  std::smatch value;
  if (std::regex_search(body, value, setting)) {
    body.replace(static_cast<std::size_t>(value.position(2)),
                 static_cast<std::size_t>(value.length(2)), requested);
  } else {
    if (!body.empty() && body.back() != '\n') body += '\n';
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
  if (!project) return false;
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

  const bool androidSupported = AndroidConfigured(*project, manifest, projectDirectory);
  if (!requestedDefault.empty()) {
    if (requestedDefault != "desktop" && requestedDefault != "android") {
      std::cerr << "[Levye] Unsupported default platform: " << requestedDefault
                << ". Choose desktop or android; iOS is unavailable.\n";
      return false;
    }
    if (requestedDefault == "android" && !androidSupported) {
      std::cerr << "[Levye] Android is not configured for this project. Run 'levye build "
                   "--android' first.\n";
      return false;
    }
    if (project->defaultPlatform != requestedDefault || !manifest["platform"]["default"].node()) {
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
                         if (updated && updated->defaultPlatform == requestedDefault) return true;
                         validationError = "Default platform validation failed.";
                         return false;
                       })) {
        std::cerr << "[Levye] Cannot update default platform: " << error << '\n';
        return false;
      }
      project = ProjectLoader::Load(path);
      if (!project) return false;
    }
  }

  std::cout << "[Levye] Project: " << project->name << '\n';
  if (const auto version = manifest["project"]["template_version"].value<int64_t>())
    std::cout << "[Levye] Template: " << *version << '\n';
  else
    std::cout << "[Levye] Template: legacy\n";
  std::cout << "[Levye] Default platform: "
            << (project->defaultPlatform == "android" ? "Android" : "Desktop") << '\n'
            << "[Levye] Supported platforms: Desktop" << (androidSupported ? ", Android" : "")
            << '\n'
            << "[Levye] Configuration: Debug\n"
            << "[Levye] Root: " << projectDirectory.string() << '\n';
  return true;
}

}  // namespace Levye
