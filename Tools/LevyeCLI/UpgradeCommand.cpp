#include "UpgradeCommand.hpp"

#include <Levye/Project/ProjectLoader.hpp>
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

constexpr int kTemplateVersion = 2;

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

// Match only the known generated host. Preserve custom source on failure.
bool PrepareHost(const fs::path& root, const fs::path& templateRoot,
                 std::vector<PendingFile>& changes, std::string& error) {
  const auto path = root / "Source/Main.cpp";
  std::string current, desired;
  if (!ReadRegular(path, current, error) ||
      !ReadRegular(templateRoot / "Source/Main.cpp", desired, error))
    return false;
  if (current == desired)
    return true;
  constexpr std::string_view legacy = R"LEVYE(#include <Levye/Core/Application.hpp>
#include <Levye/HotReload/GameModule.hpp>
#include <Levye/Platform/ExecutablePath.hpp>
#include <Levye/Project/ProjectLoader.hpp>
#include <Levye/Platform/SharedLibrary.hpp>

#include <filesystem>
#include <iostream>

int main()
{
    const std::filesystem::path executableDirectory =
        Levye::ExecutablePath::GetDirectory();

    const std::filesystem::path targetDirectory =
        executableDirectory.parent_path();

    const std::filesystem::path projectDirectory =
        targetDirectory
            .parent_path()
            .parent_path();

    const std::filesystem::path projectFile =
        projectDirectory /
        "levye.project";

    const auto project =
        Levye::ProjectLoader::Load(
            projectFile
        );

    if (!project)
    {
        std::cerr
            << "[Levye] Failed to load project.\n";

        return 1;
    }

    const std::filesystem::path assetRoot =
        projectDirectory /
        project->assetDirectory;

    const std::filesystem::path gameModulePath =
        targetDirectory /
        "lib" /
        Levye::SharedLibrary::MakeFilename("Game");

    Levye::GameModule gameModule;

    gameModule.SetAssetRoot(
        assetRoot.string()
    );

    if (!gameModule.Load(
            gameModulePath.string()))
    {
        return 1;
    }

    const Levye::ApplicationConfig applicationConfig =
        project->CreateApplicationConfig();

    Levye::Application application{
        applicationConfig,
        gameModule
    };

    application.Run();

    return 0;
})LEVYE";

// Recognize the generated host used before explicit
// application initialization was introduced.
std::string previous = desired;

constexpr std::string_view initialization =
    "  if (!application.Initialize())\n"
    "    return 1;\n\n";

const auto initializationAt = previous.find(initialization);

if (initializationAt != std::string::npos) {
  previous.erase(initializationAt, initialization.size());
}

// Ignore trailing whitespace when comparing generated hosts.
const auto normalize = [](std::string_view source) {
  std::string result;

  result.reserve(source.size());

  for (const char c : source) {
    if (c != '\r')
      result += c;
  }

  while (!result.empty() &&
         std::isspace(static_cast<unsigned char>(result.back()))) {
    result.pop_back();
  }

  return result;
};

const auto normalizedCurrent = normalize(current);

const bool knownHost =
    normalizedCurrent == normalize(desired) ||
    normalizedCurrent == normalize(legacy) ||
    normalizedCurrent == normalize(previous);

if (!knownHost) {
  error =
      "Custom Source/Main.cpp requires manual runtime migration; "
      "no files changed.";

  return false;
}
  changes.push_back({path, desired, true});
  return true;
}

bool SupportsBuildLayout(std::string& cmake, std::string_view target, bool& changed,
                         std::string& error) {
  changed = false;
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
  constexpr std::string_view legacyLayout = R"(if(WIN32)
    set(LEVYE_TARGET_DIRECTORY "${CMAKE_SOURCE_DIR}/Targets/$<CONFIG>")
else()
    # Preserve the existing macOS/Linux configuration and output layout.
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    endif()
    set(LEVYE_TARGET_DIRECTORY "${CMAKE_SOURCE_DIR}/Targets/${CMAKE_BUILD_TYPE}")
endif())";
  constexpr std::string_view buildNameLayout = R"(get_filename_component(
    LEVYE_BUILD_NAME
    "${CMAKE_BINARY_DIR}"
    NAME
)

set(
    LEVYE_OUTPUT_DIR
    "${CMAKE_SOURCE_DIR}/Targets/${LEVYE_BUILD_NAME}"
)

set(LEVYE_TARGET_DIRECTORY "${LEVYE_OUTPUT_DIR}"))";
  constexpr std::string_view simpleLegacyLayout = R"(if(NOT CMAKE_BUILD_TYPE)
    set(
        CMAKE_BUILD_TYPE
        Debug
        CACHE STRING
        "Build type"
        FORCE
    )
endif()

set(
    LEVYE_TARGET_DIRECTORY
    "${CMAKE_SOURCE_DIR}/Targets/${CMAKE_BUILD_TYPE}"
))";
  const auto old = cmake.find(legacyLayout);
  const auto simpleOld = cmake.find(simpleLegacyLayout);
  if (old != std::string::npos && simpleOld != std::string::npos) {
    error = "Multiple output layout blocks require manual review.";
    return false;
  }
  if (old != std::string::npos || simpleOld != std::string::npos) {
    cmake.replace(old != std::string::npos ? old : simpleOld,
                  old != std::string::npos ? legacyLayout.size() : simpleLegacyLayout.size(),
                  buildNameLayout);
    changed = true;
  }
  if (cmake.find(buildNameLayout) == std::string::npos ||
      cmake.find("LIBRARY_OUTPUT_DIRECTORY") == std::string::npos ||
      cmake.find("RUNTIME_OUTPUT_DIRECTORY") == std::string::npos ||
      cmake.find(R"("${LEVYE_TARGET_DIRECTORY}/bin")") == std::string::npos ||
      cmake.find(R"("${LEVYE_TARGET_DIRECTORY}/lib")") == std::string::npos) {
    error = "Unsupported output layout; expected Targets/<build directory name>.";
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
  if (version == kTemplateVersion)
    return true;

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
    if (lineEnd == std::string::npos)
      break;
    lineStart = lineEnd + 1;
  }
  std::string section = updated.substr(start, end - start);
  const std::regex field(
      R"((^|\n)[ \t]*template_version[ \t]*=[ \t]*([0-9]+)[ \t]*(?:#.*)?(?=\r?\n|$))");
  std::smatch versionMatch;
  if (std::regex_search(section, versionMatch, field)) {
    section.replace(static_cast<std::size_t>(versionMatch.position(2)),
                    static_cast<std::size_t>(versionMatch.length(2)),
                    std::to_string(kTemplateVersion));
  } else {
    if (!section.empty() && section.back() != '\n')
      section += '\n';
    section += "template_version = " + std::to_string(kTemplateVersion) + "\n";
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

bool PrepareDisplayMigration(std::string& text, std::string& error) {
  toml::table parsed;
  try {
    parsed = toml::parse(text);
  } catch (const toml::parse_error& e) {
    error = std::string(e.description());
    return false;
  }
  struct Move {
    const char* fromSection;
    const char* fromKey;
    const char* toSection;
    const char* toKey;
  };
  constexpr Move moves[] = {{"window", "target_fps", "display", "target_fps"},
                            {"window", "vsync", "display", "vsync"},
                            {"display", "viewport_width", "display", "logical_width"},
                            {"display", "viewport_height", "display", "logical_height"},
                            {"display", "viewport_mode", "display", "scale_mode"},
                            {"display", "scaling", "display", "scale_mode"},
                            {"display", "window_width", "desktop", "window_width"},
                            {"display", "window_height", "desktop", "window_height"},
                            {"ios", "orientation", "display", "orientation"},
                            {"android", "orientation", "display", "orientation"}};
  auto field = [&](std::string_view section, std::string_view key, std::size_t& start,
                   std::size_t& end, std::string& value) {
    const std::regex header("(^|\\n)[ \\t]*\\[" + std::string(section) +
                            R"(\][ \t]*(?:#.*)?\r?\n)");
    std::smatch found;
    if (!std::regex_search(text, found, header))
      return false;
    const auto bodyStart = static_cast<std::size_t>(found.position() + found.length());
    const std::regex nextTable(R"((^|\n)[ \t]*\[[^\]\r\n]+\][ \t]*(?:#.*)?\r?(?:\n|$))");
    std::smatch nextMatch;
    const auto tail = text.substr(bodyStart);
    const auto limit = std::regex_search(tail, nextMatch, nextTable)
                           ? bodyStart + static_cast<std::size_t>(nextMatch.position()) +
                                 (nextMatch[1].length() ? 1 : 0)
                           : text.size();
    const std::regex line("(^|\\n)[ \\t]*" + std::string(key) +
                          R"([ \t]*=[ \t]*([^#\r\n]+)[ \t]*(?:#.*)?(?=\r?\n|$))");
    const auto body = text.substr(bodyStart, limit - bodyStart);
    if (!std::regex_search(body, found, line))
      return false;
    start = bodyStart + static_cast<std::size_t>(found.position()) + (found[1].length() ? 1 : 0);
    end = bodyStart + static_cast<std::size_t>(found.position() + found.length());
    value = found[2].str();
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
      value.pop_back();
    return true;
  };
  for (const auto& move : moves) {
    std::size_t sourceStart, sourceEnd, targetStart, targetEnd;
    std::string sourceValue, targetValue;
    if (!field(move.fromSection, move.fromKey, sourceStart, sourceEnd, sourceValue))
      continue;
    if (std::string_view(move.fromSection) == "ios" && sourceValue == "\"all\"")
      sourceValue = "\"auto\"";
    if (field(move.toSection, move.toKey, targetStart, targetEnd, targetValue)) {
      const auto sourceNode = parsed[move.fromSection][move.fromKey];
      const auto targetNode = parsed[move.toSection][move.toKey];
      const bool sameValue = [&] {
        if (const auto source = sourceNode.value<std::string>()) {
          const auto target = targetNode.value<std::string>();
          return target && (*target == *source || (std::string_view(move.fromSection) == "ios" &&
                                                   *source == "all" && *target == "auto"));
        }
        if (const auto source = sourceNode.value<std::int64_t>())
          return targetNode.value<std::int64_t>() == source;
        if (const auto source = sourceNode.value<bool>())
          return targetNode.value<bool>() == source;
        return false;
      }();
      if (!sameValue) {
        error = "Conflicting " + std::string(move.fromSection) + "." + move.fromKey + " and " +
                move.toSection + "." + move.toKey + ".";
        return false;
      }
      text.erase(sourceStart, sourceEnd - sourceStart);
      continue;
    }
    if (std::string_view(move.fromSection) == move.toSection) {
      const auto line = text.substr(sourceStart, sourceEnd - sourceStart);
      const auto keyAt = line.find(move.fromKey);
      text.replace(sourceStart + keyAt, std::char_traits<char>::length(move.fromKey), move.toKey);
      continue;
    }
    std::string movedLine = text.substr(sourceStart, sourceEnd - sourceStart);
    const auto keyAt = movedLine.find(move.fromKey);
    movedLine.replace(keyAt, std::char_traits<char>::length(move.fromKey), move.toKey);
    if (std::string_view(move.fromSection) == "ios" && sourceValue == "\"auto\"") {
      const auto oldValue = movedLine.find("\"all\"");
      if (oldValue != std::string::npos)
        movedLine.replace(oldValue, 5, "\"auto\"");
    }
    text.erase(sourceStart, sourceEnd - sourceStart);
    const std::regex targetHeader("(^|\\n)[ \\t]*\\[" + std::string(move.toSection) +
                                  R"(\][ \t]*(?:#.*)?\r?(?:\n|$))");
    std::smatch targetMatch;
    if (!std::regex_search(text, targetMatch, targetHeader)) {
      if (!text.empty() && text.back() != '\n')
        text += '\n';
      text += "\n[" + std::string(move.toSection) + "]\n" + movedLine + "\n";
    } else {
      const auto at = static_cast<std::size_t>(targetMatch.position() + targetMatch.length());
      text.insert(at, movedLine + "\n");
    }
  }
  // Drop an empty legacy [window] table after its last settings were moved.
  // Preserve tables that still contain width, height, resizable, or custom keys.
  text = std::regex_replace(
      text, std::regex(R"((^|\n)[ \t]*\[window\][ \t]*(?:\#[^\n]*)?\n(?=[ \t]*(?:\n|\[|$)))"),
      "$1");
  if (!parsed["display"].node() && text.find("[display]") != std::string::npos) {
    std::size_t start, end;
    std::string orientation;
    const bool portrait =
        field("display", "orientation", start, end, orientation) && orientation == "\"portrait\"";
    const int width = parsed["window"]["width"].value<int>().value_or(portrait ? 720 : 1280);
    const int height = parsed["window"]["height"].value<int>().value_or(portrait ? 1280 : 720);
    const auto at = text.find('[', text.find("[display]")) + std::string("[display]").size();
    const auto lineEnd = text.find('\n', at);
    text.insert(lineEnd + 1, "logical_width = " + std::to_string(width) + "\nlogical_height = " +
                                 std::to_string(height) + "\nscale_mode = \"fit\"\n");
  }
  try {
    (void)toml::parse(text);
  } catch (const toml::parse_error& e) {
    error = "Display migration produced invalid TOML: " + std::string(e.description());
    return false;
  }
  if (!ProjectLoader::Parse(text)) {
    error = "Display migration contains unsupported or conflicting settings.";
    return false;
  }
  return true;
}

bool ValidateInstalled(const fs::path& root, const fs::path& templateRoot, std::string& error) {
  std::string metadata;
  if (!ReadRegular(root / "levye.project", metadata, error))
    return false;
  std::string updated, name, target;
  int version = 0;
  if (!PrepareMetadata(metadata, updated, name, target, version, error) ||
      version != kTemplateVersion) {
    if (error.empty())
      error = "Project template version was not installed.";
    return false;
  }
  if (!PrepareDisplayMigration(updated, error) || updated != metadata) {
    if (error.empty())
      error = "Display migration is incomplete.";
    return false;
  }
  std::vector<PendingFile> hostChanges;
  if (!PrepareHost(root, templateRoot, hostChanges, error))
    return false;
  if (!hostChanges.empty()) {
    error = "Desktop runtime host migration is incomplete.";
    return false;
  }
  CMakeMigrationPlan source;
  if (!ProjectCMakeMigration::Prepare(root, source, error) || source.updateRoot ||
      source.createManifest || source.updateManifest) {
    if (error.empty())
      error = "Source manifest migration is incomplete.";
    return false;
  }
  std::string cmake;
  if (!ReadRegular(root / "CMakeLists.txt", cmake, error))
    return false;
  bool layoutChanged = false;
  if (!SupportsBuildLayout(cmake, target, layoutChanged, error) || layoutChanged)
    return false;
  bool modulesChanged = false;
  const std::regex compileCommandsOn(R"(^[ \t]*set\s*\(\s*CMAKE_EXPORT_COMPILE_COMMANDS\s+ON\s*\))",
                                     std::regex::icase | std::regex_constants::multiline);
  if (!ProjectModuleMigration::Prepare(cmake, modulesChanged, error) || modulesChanged ||
      !std::regex_search(cmake, compileCommandsOn)) {
    if (error.empty())
      error = "CMake migration is incomplete.";
    return false;
  }
  std::vector<PendingFile> missing;
  std::vector<fs::path> preserved;
  if (!ProjectIDE::Prepare(root, templateRoot, target, missing, preserved, error))
    return false;
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
  if (!PrepareDisplayMigration(updatedMetadata, error)) {
    std::cerr << "[Levye] Upgrade failed: " << error << '\n';
    return 1;
  }
  std::cout << "[Levye] Project: " << name << '\n'
            << "[Levye] Current template: " << (version == 0 ? "legacy" : std::to_string(version))
            << '\n'
            << "[Levye] Target template: " << kTemplateVersion << '\n';

  std::vector<PendingFile> changes;
  const auto templateRoot = frameworkRoot / "Templates/Default";
  if (!PrepareHost(projectDirectory, templateRoot, changes, error)) {
    std::cerr << "[Levye] Upgrade failed: " << error << '\n';
    return 1;
  }

  CMakeMigrationPlan source;
  if (!ProjectCMakeMigration::Prepare(projectDirectory, source, error)) {
    std::cerr << "[Levye] Upgrade failed: " << error << '\n';
    return 1;
  }
  bool layoutChanged = false;
  if (!SupportsBuildLayout(source.rootCMake, target, layoutChanged, error)) {
    std::cerr << "[Levye] Upgrade failed: " << error << '\n';
    return 1;
  }
  source.updateRoot |= layoutChanged;
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

  if (source.updateRoot)
    changes.push_back({projectDirectory / "CMakeLists.txt", source.rootCMake, true});
  if (source.createManifest)
    changes.push_back({projectDirectory / "Source/CMakeLists.txt", source.sourceManifest, false});
  if (source.updateManifest)
    changes.push_back({projectDirectory / "Source/CMakeLists.txt", source.sourceManifest, true});

  std::vector<fs::path> preserved;
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
  if (changes.empty())
    std::cout << "  (none)\n";
  for (const auto& change : changes)
    std::cout << "  " << (change.allowReplace ? "~ " : "+ ")
              << change.path.lexically_relative(projectDirectory).generic_string() << '\n';
  std::cout << "[Levye] Preserved:\n";
  for (const auto& file : preserved)
    std::cout << "  " << file.generic_string() << '\n';
  std::cout << "  Game sources and Assets/ contents, target names, custom project settings\n";

  if (check)
    return changes.empty() ? 0 : 2;
  if (dryRun)
    return 0;
  if (changes.empty()) {
    std::cout << "[Levye] Project is current.\n";
    return 0;
  }
  // Keep a recoverable copy of every file that the transaction will replace.
  fs::path backupRoot = projectDirectory / ".levye-upgrade-backup";
  for (int suffix = 2; fs::exists(backupRoot); ++suffix)
    backupRoot = projectDirectory / (".levye-upgrade-backup-" + std::to_string(suffix));
  std::error_code backupError;
  if (!fs::create_directory(backupRoot, backupError) || backupError) {
    std::cerr << "[Levye] Upgrade failed: cannot create backup directory.\n";
    return 1;
  }
  for (const auto& change : changes) {
    if (!change.allowReplace)
      continue;
    const auto backup = backupRoot / change.path.lexically_relative(projectDirectory);
    fs::create_directories(backup.parent_path(), backupError);
    if (backupError || !fs::copy_file(change.path, backup, fs::copy_options::none, backupError)) {
      std::cerr << "[Levye] Upgrade failed: cannot back up " << change.path << ".\n";
      return 1;
    }
  }
  std::cout << "[Levye] Backup: " << backupRoot << '\n';
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
