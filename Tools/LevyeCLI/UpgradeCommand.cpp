#include "UpgradeCommand.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <string>
#include <toml++/toml.hpp>
#include <vector>

#include "FileTransaction.hpp"
#include "ProjectCMakeMigration.hpp"
#include "ProjectIDE.hpp"
#include "ProjectModuleMigration.hpp"

namespace Levye {
namespace {
namespace fs = std::filesystem;

constexpr int kTemplateVersion = 1;

bool ReadRegular(const fs::path& path, std::string& text, std::string& error) {
  std::error_code ec;
  if (!fs::is_regular_file(fs::symlink_status(path, ec)) || ec) {
    error = "Missing or unsafe file: " + path.string();
    return false;
  }
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    error = "Cannot read: " + path.string();
    return false;
  }
  text.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
  if (input.bad()) {
    error = "Cannot read: " + path.string();
    return false;
  }
  return true;
}

bool SupportsBuildLayout(const std::string& cmake, std::string_view target, std::string& error) {
  const std::regex projectName(R"(^[ \t]*project\s*\(\s*([A-Za-z0-9_.+-]+)\s+LANGUAGES\b)",
                               std::regex::icase | std::regex_constants::multiline);
  std::smatch projectMatch;
  if (!std::regex_search(cmake, projectMatch, projectName) ||
      projectMatch[1].str() != std::string(target)) {
    error = "Project CMake target differs from levye.project or uses an unsupported expression.";
    return false;
  }
  const std::regex host(
      R"(^[ \t]*add_executable\s*\(\s*([A-Za-z0-9_.+-]+|\$\{PROJECT_NAME\})\s+Source/Main\.cpp\s*\))",
      std::regex::icase | std::regex_constants::multiline);
  std::smatch match;
  if (!std::regex_search(cmake, match, host) ||
      (match[1].str() != std::string(target) && match[1] != "${PROJECT_NAME}")) {
    error = "Unsupported host executable layout; expected the project target and Source/Main.cpp.";
    return false;
  }
  if (cmake.find(R"("${CMAKE_SOURCE_DIR}/Targets/$<CONFIG>")") == std::string::npos ||
      cmake.find(R"("${CMAKE_SOURCE_DIR}/Targets/${CMAKE_BUILD_TYPE}")") == std::string::npos ||
      cmake.find("LIBRARY_OUTPUT_DIRECTORY") == std::string::npos ||
      cmake.find("RUNTIME_OUTPUT_DIRECTORY") == std::string::npos ||
      cmake.find(R"("${LEVYE_TARGET_DIRECTORY}/bin")") == std::string::npos ||
      cmake.find(R"("${LEVYE_TARGET_DIRECTORY}/lib")") == std::string::npos) {
    error = "Unsupported output layout; levye build/run expect Targets/<configuration>.";
    return false;
  }
  const std::regex imguiLink(R"(^[ \t]*target_link_libraries\s*\(\s*Game\b[^)]*\bLevyeDearImGui\b)",
                             std::regex::icase | std::regex_constants::multiline);
  if (!std::regex_search(cmake, imguiLink) ||
      cmake.find("WIN32 AND LEVYE_WITH_IMGUI") == std::string::npos ||
      cmake.find("$<TARGET_FILE:LevyeDearImGui>") == std::string::npos) {
    error = "Unsupported ImGui game linkage or Windows runtime deployment layout.";
    return false;
  }
  return true;
}

bool PrepareMetadata(const std::string& original, std::string& updated, std::string& name,
                     std::string& target, int& version, std::string& error) {
  toml::table config;
  try {
    config = toml::parse(original);
  } catch (const toml::parse_error& parseError) {
    error = "Invalid levye.project: " + std::string(parseError.description());
    return false;
  }
  const auto* project = config["project"].as_table();
  if (!project || !config["project"]["name"].is_string() ||
      !config["project"]["target"].is_string()) {
    error = "levye.project needs project.name and project.target strings.";
    return false;
  }
  name = config["project"]["name"].value<std::string>().value_or("");
  target = config["project"]["target"].value<std::string>().value_or("");
  if (name.empty() || target.empty()) {
    error = "Project name and target must not be empty.";
    return false;
  }
  if (!std::all_of(target.begin(), target.end(), [](unsigned char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
               c == '_' || c == '-' || c == '.' || c == '+';
      })) {
    error = "Project target contains characters unsafe for IDE configuration.";
    return false;
  }
  version = 0;
  if (const auto* value = project->get("template_version")) {
    const auto parsed = value->value<int64_t>();
    if (!parsed || *parsed < 0 || *parsed > kTemplateVersion) {
      error = "Unsupported future or invalid project template version.";
      return false;
    }
    version = static_cast<int>(*parsed);
  }
  updated = original;
  if (version == kTemplateVersion) return true;

  const std::regex header(R"((^|\n)[ \t]*\[project\][ \t]*(?:#.*)?\r?\n)");
  std::smatch match;
  if (!std::regex_search(updated, match, header)) {
    error = "Cannot locate [project] section for template version.";
    return false;
  }
  const auto start = static_cast<std::size_t>(match.position() + match.length());
  const std::regex table(R"(^[ \t]*(?:\[\[[^\]\r\n]+\]\]|\[[^\]\r\n]+\])[ \t]*(?:#.*)?\r?$)");
  std::size_t end = updated.size();
  for (std::size_t lineStart = start; lineStart < updated.size();) {
    const auto lineEnd = updated.find('\n', lineStart);
    if (std::regex_match(updated.substr(lineStart, lineEnd - lineStart), table)) {
      end = lineStart;
      break;
    }
    if (lineEnd == std::string::npos) break;
    lineStart = lineEnd + 1;
  }
  std::string section = updated.substr(start, end - start);
  const std::regex field(
      R"((^|\n)[ \t]*template_version[ \t]*=[ \t]*([0-9]+)[ \t]*(?:#.*)?(?=\r?\n|$))");
  std::smatch versionMatch;
  if (std::regex_search(section, versionMatch, field)) {
    section.replace(static_cast<std::size_t>(versionMatch.position(2)),
                    static_cast<std::size_t>(versionMatch.length(2)), "1");
  } else {
    if (!section.empty() && section.back() != '\n') section += '\n';
    section += "template_version = 1\n";
  }
  updated.replace(start, end - start, section);
  try {
    (void)toml::parse(updated);
  } catch (const toml::parse_error&) {
    error = "Template version update would invalidate levye.project.";
    return false;
  }
  return true;
}

bool ValidateInstalled(const fs::path& root, const fs::path& templateRoot, std::string& error) {
  std::string metadata;
  if (!ReadRegular(root / "levye.project", metadata, error)) return false;
  std::string updated, name, target;
  int version = 0;
  if (!PrepareMetadata(metadata, updated, name, target, version, error) ||
      version != kTemplateVersion) {
    if (error.empty()) error = "Project template version was not installed.";
    return false;
  }
  CMakeMigrationPlan source;
  if (!ProjectCMakeMigration::Prepare(root, source, error) || source.updateRoot ||
      source.createManifest || source.updateManifest) {
    if (error.empty()) error = "Source manifest migration is incomplete.";
    return false;
  }
  std::string cmake;
  if (!ReadRegular(root / "CMakeLists.txt", cmake, error)) return false;
  if (!SupportsBuildLayout(cmake, target, error)) return false;
  bool modulesChanged = false;
  const std::regex compileCommandsOn(R"(^[ \t]*set\s*\(\s*CMAKE_EXPORT_COMPILE_COMMANDS\s+ON\s*\))",
                                     std::regex::icase | std::regex_constants::multiline);
  if (!ProjectModuleMigration::Prepare(cmake, modulesChanged, error) || modulesChanged ||
      !std::regex_search(cmake, compileCommandsOn)) {
    if (error.empty()) error = "CMake migration is incomplete.";
    return false;
  }
  std::vector<PendingFile> missing;
  std::vector<fs::path> preserved;
  if (!ProjectIDE::Prepare(root, templateRoot, target, missing, preserved, error)) return false;
  if (!missing.empty()) {
    error = "IDE configuration is incomplete.";
    return false;
  }
  return true;
}

bool ValidatePlanPaths(const fs::path& root, const std::vector<PendingFile>& changes,
                       std::string& error) {
  std::error_code rootError;
  if (!fs::is_directory(fs::symlink_status(root, rootError)) || rootError) {
    error = "Project root is missing or unsafe.";
    return false;
  }
  for (const auto& change : changes) {
    const auto relative = change.path.lexically_normal().lexically_relative(root);
    if (relative.empty() || relative == "." || relative.is_absolute() ||
        *relative.begin() == "..") {
      error = "Upgrade destination escapes the project: " + change.path.string();
      return false;
    }
    fs::path current = root;
    for (const auto& component : relative) {
      current /= component;
      std::error_code ec;
      const auto status = fs::symlink_status(current, ec);
      if (ec && ec != std::errc::no_such_file_or_directory) {
        error = "Cannot inspect upgrade destination: " + current.string();
        return false;
      }
      if (!ec && fs::is_symlink(status)) {
        error = "Upgrade destination contains a symlink: " + current.string();
        return false;
      }
      if (!ec && current != change.path && !fs::is_directory(status)) {
        error = "Upgrade destination parent is not a directory: " + current.string();
        return false;
      }
    }
  }
  return true;
}
}  // namespace

int UpgradeCommand::Execute(const fs::path& projectDirectory, const fs::path& frameworkRoot,
                            bool dryRun, bool check) {
  std::cout << "[Levye] Project upgrade\n";
  std::string error, metadata, updatedMetadata, name, target;
  int version = 0;
  if (!ReadRegular(projectDirectory / "levye.project", metadata, error) ||
      !PrepareMetadata(metadata, updatedMetadata, name, target, version, error)) {
    std::cerr << "[Levye] Upgrade failed: " << error << '\n';
    return 1;
  }
  std::cout << "[Levye] Project: " << name << '\n'
            << "[Levye] Current template: " << (version == 0 ? "legacy" : std::to_string(version))
            << '\n'
            << "[Levye] Target template: " << kTemplateVersion << '\n';

  std::string hostSource;
  if (!ReadRegular(projectDirectory / "Source/Main.cpp", hostSource, error)) {
    std::cerr << "[Levye] Upgrade failed: " << error << '\n';
    return 1;
  }

  CMakeMigrationPlan source;
  if (!ProjectCMakeMigration::Prepare(projectDirectory, source, error)) {
    std::cerr << "[Levye] Upgrade failed: " << error << '\n';
    return 1;
  }
  if (!SupportsBuildLayout(source.rootCMake, target, error)) {
    std::cerr << "[Levye] Upgrade failed: " << error << '\n';
    return 1;
  }
  bool modulesChanged = false;
  if (!ProjectModuleMigration::Prepare(source.rootCMake, modulesChanged, error)) {
    std::cerr << "[Levye] Upgrade failed: " << error << '\n';
    return 1;
  }
  source.updateRoot |= modulesChanged;
  const std::regex compileCommands(
      R"(^[ \t]*set\s*\(\s*CMAKE_EXPORT_COMPILE_COMMANDS\s+([^\s)]+)\s*\))",
      std::regex::icase | std::regex_constants::multiline);
  std::smatch compileMatch;
  if (std::regex_search(source.rootCMake, compileMatch, compileCommands)) {
    std::string setting = compileMatch[1].str();
    std::transform(setting.begin(), setting.end(), setting.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    if (setting != "ON") {
      std::cerr << "[Levye] Upgrade failed: custom compile commands setting requires review.\n";
      return 1;
    }
  } else {
    const std::regex minimum(R"(^[ \t]*cmake_minimum_required\s*\([^)]*\))",
                             std::regex::icase | std::regex_constants::multiline);
    std::smatch minimumMatch;
    if (!std::regex_search(source.rootCMake, minimumMatch, minimum)) {
      std::cerr << "[Levye] Upgrade failed: CMake minimum version declaration is missing.\n";
      return 1;
    }
    const auto after = static_cast<std::size_t>(minimumMatch.position() + minimumMatch.length());
    source.rootCMake.insert(after, "\n\nset(CMAKE_EXPORT_COMPILE_COMMANDS ON)");
    source.updateRoot = true;
  }

  std::vector<PendingFile> changes;
  if (source.updateRoot)
    changes.push_back({projectDirectory / "CMakeLists.txt", source.rootCMake, true});
  if (source.createManifest)
    changes.push_back({projectDirectory / "Source/CMakeLists.txt", source.sourceManifest, false});
  if (source.updateManifest)
    changes.push_back({projectDirectory / "Source/CMakeLists.txt", source.sourceManifest, true});

  std::vector<fs::path> preserved;
  const auto templateRoot = frameworkRoot / "Templates/Default";
  if (!ProjectIDE::Prepare(projectDirectory, templateRoot, target, changes, preserved, error)) {
    std::cerr << "[Levye] Upgrade failed: " << error << '\n';
    return 1;
  }
  // Install the version marker last, after all structural files in the same transaction.
  if (updatedMetadata != metadata)
    changes.push_back({projectDirectory / "levye.project", updatedMetadata, true});

  if (!ValidatePlanPaths(projectDirectory, changes, error)) {
    std::cerr << "[Levye] Upgrade failed: " << error << '\n';
    return 1;
  }
  if (changes.empty() && !ValidateInstalled(projectDirectory, templateRoot, error)) {
    std::cerr << "[Levye] Upgrade failed: " << error << '\n';
    return 1;
  }

  std::cout << "[Levye] Planned changes:\n";
  if (changes.empty()) std::cout << "  (none)\n";
  for (const auto& change : changes)
    std::cout << "  " << (change.allowReplace ? "~ " : "+ ")
              << change.path.lexically_relative(projectDirectory).generic_string() << '\n';
  std::cout << "[Levye] Preserved:\n";
  for (const auto& file : preserved) std::cout << "  " << file.generic_string() << '\n';
  std::cout << "  Source/ and Assets/ contents, target names, custom project settings\n";

  if (check) return changes.empty() ? 0 : 2;
  if (dryRun) return 0;
  if (changes.empty()) {
    std::cout << "[Levye] Project is current.\n";
    return 0;
  }
  if (!CommitFiles(projectDirectory, changes, error, nullptr, [&](std::string& validationError) {
        return ValidateInstalled(projectDirectory, templateRoot, validationError);
      })) {
    std::cerr << "[Levye] Upgrade failed: " << error << '\n';
    return 1;
  }
  std::cout << "[Levye] Upgrade completed (structural validation; build not run).\n";
  return 0;
}

}  // namespace Levye
