#include "ConvertCommand.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <toml++/toml.hpp>
#include <vector>

#include "FileTransaction.hpp"
#include "ProjectAnalyzer.hpp"
#include "ProjectCMakeMigration.hpp"
#include "ProjectGenerator.hpp"
#include "ProjectIDE.hpp"
#include "ProjectModuleMigration.hpp"

namespace Levye {
namespace {
namespace fs = std::filesystem;

struct ConversionPlan {
  std::vector<PendingFile> files;
  std::vector<fs::path> preserved;
  std::vector<std::string> notes;
  std::string error;
};

bool ReadRegular(const fs::path& path, std::string& text) {
  std::error_code ec;
  if (!fs::is_regular_file(fs::symlink_status(path, ec)) || ec) return false;
  std::ifstream input(path, std::ios::binary);
  if (!input) return false;
  text.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
  return !input.bad();
}

bool ReplaceOnce(std::string& text, std::string_view oldValue, std::string_view newValue) {
  const auto at = text.find(oldValue);
  if (at == std::string::npos || text.find(oldValue, at + oldValue.size()) != std::string::npos)
    return false;
  text.replace(at, oldValue.size(), newValue);
  return true;
}

bool SafeMissing(const fs::path& path) {
  std::error_code ec;
  const auto status = fs::symlink_status(path, ec);
  return ec == std::errc::no_such_file_or_directory && !fs::exists(status);
}

bool ValidateDestinations(const fs::path& root, const std::vector<PendingFile>& files,
                          std::string& error) {
  for (const auto& file : files) {
    const auto relative = file.path.lexically_normal().lexically_relative(root);
    if (relative.empty() || relative.is_absolute() || *relative.begin() == "..") {
      error = "Output escapes the project: " + file.path.string();
      return false;
    }
    fs::path current = root;
    for (const auto& part : relative) {
      current /= part;
      std::error_code ec;
      const auto status = fs::symlink_status(current, ec);
      if (ec && ec != std::errc::no_such_file_or_directory) {
        error = "Cannot inspect output path: " + current.string();
        return false;
      }
      if (!ec && fs::is_symlink(status)) {
        error = "Output path contains a symlink: " + current.string();
        return false;
      }
      if (current == file.path) {
        if (file.allowReplace ? (ec || !fs::is_regular_file(status))
                              : ec != std::errc::no_such_file_or_directory) {
          error = "Output path conflicts with an existing file: " + current.string();
          return false;
        }
      } else if (!ec && !fs::is_directory(status)) {
        error = "Output parent is not a directory: " + current.string();
        return false;
      }
    }
  }
  return true;
}

std::string CMakeQuoted(const fs::path& path) { return "\"" + path.generic_string() + "\""; }

std::string CMakeList(std::string_view command, std::string_view target,
                      const std::vector<std::string>& values) {
  if (values.empty()) return {};
  std::string text = "\n" + std::string(command) + "(" + std::string(target) + " PRIVATE";
  for (const auto& value : values) text += " \"" + value + "\"";
  text += ")\n";
  return text;
}

bool RenderGame(std::string& text, const ConversionAnalysis& analysis) {
  if (analysis.lifecycle != ConversionLifecycle::RaylibApplication) return true;
  const auto name = text.find("void OnDraw(");
  if (name == std::string::npos) return false;
  const auto open = text.find('{', name);
  const auto close = text.find('}', open);
  if (open == std::string::npos || close == std::string::npos ||
      text.substr(open, close - open).find("ClearBackground(BLACK)") == std::string::npos)
    return false;
  text.replace(open + 1, close - open - 1, "\n    (void)state;\n\n" + analysis.drawCalls + "\n");
  return true;
}

bool PlanConversion(const ConversionAnalysis& analysis, const fs::path& templateRoot,
                    ConversionPlan& plan) {
  const auto& root = analysis.projectRoot;
  const auto& original = analysis.targets.front();
  const bool library = original.type != ConversionProjectType::Executable;
  const std::string hostTarget = library && analysis.projectName == original.name
                                     ? analysis.projectName + "Host"
                                     : analysis.projectName;
  if (hostTarget == "Game" || hostTarget == original.name && library) {
    plan.error = "Generated host target conflicts with the original library.";
    return false;
  }
  if (!SafeMissing(root / "levye.project") || !SafeMissing(root / "Source/CMakeLists.txt") ||
      !SafeMissing(root / "CMakeLists.levye-original.txt")) {
    plan.error = "A generated project or original-CMake backup path already exists.";
    return false;
  }
  const fs::path gameFile = SafeMissing(root / "Source/Game.cpp")
                                ? fs::path("Source/Game.cpp")
                                : fs::path("Source/LevyeConvertedGame.cpp");
  const fs::path hostFile = SafeMissing(root / "Source/Main.cpp")
                                ? fs::path("Source/Main.cpp")
                                : fs::path("Source/LevyeHost.cpp");
  if (!SafeMissing(root / gameFile) || !SafeMissing(root / hostFile)) {
    plan.error = "Generated source filename conflicts with an existing file.";
    return false;
  }

  std::string cmake, manifest, config, game, host, ignore, assetMarker;
  if (!ReadRegular(templateRoot / "CMakeLists.txt", cmake) ||
      !ReadRegular(templateRoot / "Source/CMakeLists.txt", manifest) ||
      !ReadRegular(templateRoot / "levye.project", config) ||
      !ReadRegular(templateRoot / "Source/Game.cpp", game) ||
      !ReadRegular(templateRoot / "Source/Main.cpp", host) ||
      !ReadRegular(templateRoot / ".gitignore", ignore) ||
      !ReadRegular(templateRoot / "Assets/.empty", assetMarker)) {
    plan.error = "A required LevyeKit project template file is missing.";
    return false;
  }
  cmake = ProjectGenerator::RenderText(std::move(cmake), analysis.projectName, hostTarget,
                                       "@LEVYE_ROOT@");
  config = ProjectGenerator::RenderText(std::move(config), analysis.projectName, hostTarget, {});
  game = ProjectGenerator::RenderText(std::move(game), analysis.projectName, hostTarget, {});
  const std::string portableRoot =
      "if(NOT LEVYE_ROOT)\n"
      "    if(DEFINED ENV{LEVYE_ROOT})\n"
      "        set(LEVYE_ROOT \"$ENV{LEVYE_ROOT}\")\n"
      "    else()\n"
      "        message(FATAL_ERROR \"Set LEVYE_ROOT or use levye build.\")\n"
      "    endif()\n"
      "endif()";
  if (!ReplaceOnce(cmake, "set(\n    LEVYE_ROOT\n    \"@LEVYE_ROOT@\"\n)", portableRoot) ||
      !ReplaceOnce(cmake, "${CMAKE_CURRENT_SOURCE_DIR}/Source/Game.cpp",
                   "${CMAKE_CURRENT_SOURCE_DIR}/" + gameFile.generic_string()) ||
      !ReplaceOnce(cmake, "    Source/Main.cpp\n)", "    " + hostFile.generic_string() + "\n)")) {
    plan.error = "Current CMake template has an unsupported layout.";
    return false;
  }
  if (!RenderGame(game, analysis)) {
    plan.error = "Current game template has an unsupported drawing callback.";
    return false;
  }
  if (analysis.lifecycle == ConversionLifecycle::RaylibApplication) {
    if (!ReplaceOnce(config, "width = 1280", "width = " + std::to_string(analysis.windowWidth)) ||
        !ReplaceOnce(config, "height = 720", "height = " + std::to_string(analysis.windowHeight)) ||
        !ReplaceOnce(config, "target_fps = 60",
                     "target_fps = " + std::to_string(analysis.targetFPS)) ||
        !ReplaceOnce(config, "resizable = true", "resizable = false")) {
      plan.error = "Current project manifest template has an unsupported window layout.";
      return false;
    }
    plan.notes.push_back(
        "InitWindow/CloseWindow and BeginDrawing/EndDrawing become host lifecycle calls.");
    plan.notes.push_back(
        "Validated drawing calls move to GameAPI.OnDraw; OnUpdate and OnFixedUpdate are empty.");
  } else {
    plan.notes.push_back(
        "Original library remains a separate CMake target; the generated game is a starter "
        "module.");
  }
  if (analysis.assetDirectory != "Assets" &&
      !ReplaceOnce(config, "assets = \"Assets\"",
                   "assets = \"" + analysis.assetDirectory.generic_string() + "\"")) {
    plan.error = "Current project manifest template has an unsupported assets setting.";
    return false;
  }
  try {
    (void)toml::parse(config);
  } catch (const toml::parse_error&) {
    plan.error = "Generated levye.project is invalid.";
    return false;
  }

  if (library) {
    cmake += "\n# Preserved original library target.\nadd_library(" + original.name +
             (original.type == ConversionProjectType::StaticLibrary ? " STATIC\n" : " SHARED\n");
    for (const auto& source : original.sources) cmake += "    " + CMakeQuoted(source) + "\n";
    cmake += ")\n";
    for (const auto& setting : original.cmakeSettings) cmake += setting + "\n";
    cmake += "add_dependencies(Game " + original.name + ")\n";
  } else {
    cmake += "\n# Preserved compile settings from the original raylib target.\n";
    cmake += CMakeList("target_include_directories", "Game", original.includeDirectories);
    cmake += CMakeList("target_compile_definitions", "Game", original.compileDefinitions);
    cmake += CMakeList("target_compile_options", "Game", original.compileOptions);
  }
  plan.files.push_back({root / "CMakeLists.levye-original.txt", analysis.originalCMake, false});
  plan.files.push_back({root / "CMakeLists.txt", std::move(cmake), true});
  plan.files.push_back({root / "levye.project", std::move(config), false});
  plan.files.push_back({root / "Source/CMakeLists.txt", std::move(manifest), false});
  plan.files.push_back({root / gameFile, std::move(game), false});
  plan.files.push_back({root / hostFile, std::move(host), false});
  if (SafeMissing(root / ".gitignore"))
    plan.files.push_back({root / ".gitignore", std::move(ignore), false});
  else if (fs::is_regular_file(fs::symlink_status(root / ".gitignore")))
    plan.preserved.emplace_back(".gitignore");
  else {
    plan.error = "Existing .gitignore is not a regular file.";
    return false;
  }
  if (analysis.assetDirectory == "Assets" && SafeMissing(root / "Assets"))
    plan.files.push_back({root / "Assets/.empty", std::move(assetMarker), false});
  else
    plan.preserved.push_back(analysis.assetDirectory);
  plan.preserved.push_back(analysis.entryPoint.empty() ? fs::path("Original library sources")
                                                       : analysis.entryPoint);
  std::string error;
  if (!ProjectIDE::Prepare(root, templateRoot, hostTarget, plan.files, plan.preserved, error)) {
    plan.error = error;
    return false;
  }
  if (!ValidateDestinations(root, plan.files, plan.error)) return false;
  return true;
}

bool ValidateCommitted(const ConversionPlan& plan, const fs::path& root, std::string& error) {
  for (const auto& file : plan.files) {
    std::string actual;
    if (!ReadRegular(file.path, actual) || actual != file.content) {
      error = "Converted file failed validation: " + file.path.string();
      return false;
    }
  }
  std::string game, manifest, config;
  for (const auto& file : plan.files) {
    if (file.path.filename() == "levye.project") config = file.content;
    if (file.path.filename() == "CMakeLists.txt" && file.path.parent_path() == root / "Source")
      manifest = file.content;
    if (file.path.filename() == "Game.cpp" || file.path.filename() == "LevyeConvertedGame.cpp")
      game = file.content;
  }
  if (game.find("GetGameAPI()") == std::string::npos ||
      game.find("BindServices") == std::string::npos ||
      manifest.find("LEVYE_GENERATED_SOURCES_BEGIN") == std::string::npos) {
    error = "Generated game API or source manifest is incomplete.";
    return false;
  }
  try {
    const auto parsed = toml::parse(config);
    if (!parsed["project"]["template_version"].is_integer()) {
      error = "Generated project template version is missing.";
      return false;
    }
  } catch (const toml::parse_error&) {
    error = "Generated project configuration is invalid.";
    return false;
  }
  return true;
}
}  // namespace

int ConvertCommand::Execute(const fs::path& projectDirectory, const fs::path& frameworkRoot,
                            bool dryRun, bool check) {
  const auto analysis = ProjectAnalyzer::Analyze(projectDirectory);
  std::cout << "[Levye] Conversion analysis\n"
            << "[Levye] Project: " << analysis.projectRoot << '\n';
  if (analysis.alreadyLevyeKit) {
    std::cout << "[Levye] Already a LevyeKit project. Use levye upgrade.\n";
    return 0;
  }
  for (const auto& warning : analysis.warnings) std::cout << "[Levye] Warning: " << warning << '\n';
  if (!analysis.CanConvert()) {
    for (const auto& error : analysis.errors) std::cerr << "[Levye] Unsupported: " << error << '\n';
    return 1;
  }
  const auto& target = analysis.targets.front();
  std::cout << "[Levye] Name: " << analysis.projectName << '\n'
            << "[Levye] Target: " << target.name << " (" << target.sources.size()
            << " source files)\n"
            << "[Levye] Type: "
            << (target.type == ConversionProjectType::Executable      ? "raylib executable"
                : target.type == ConversionProjectType::StaticLibrary ? "static library"
                                                                      : "shared library")
            << '\n';
  ConversionPlan plan;
  if (!PlanConversion(analysis, frameworkRoot / "Templates/Default", plan)) {
    std::cerr << "[Levye] Unsupported: " << plan.error << '\n';
    return 1;
  }
  std::cout << "[Levye] Conversion plan:\n";
  for (const auto& file : plan.files)
    std::cout << "  " << (file.allowReplace ? "MODIFY  " : "CREATE  ")
              << file.path.lexically_relative(analysis.projectRoot).generic_string() << '\n';
  for (const auto& path : plan.preserved)
    std::cout << "  PRESERVE  " << path.generic_string() << '\n';
  for (const auto& note : plan.notes) std::cout << "[Levye] " << note << '\n';
  std::cout << "[Levye] Status: Ready for conversion\n";
  if (check) return 2;
  if (dryRun) {
    std::cout << "[Levye] No files modified.\n";
    return 0;
  }
  std::string error;
  if (!CommitFiles(analysis.projectRoot, plan.files, error, nullptr,
                   [&](std::string& validationError) {
                     return ValidateCommitted(plan, analysis.projectRoot, validationError);
                   })) {
    std::cerr << "[Levye] Conversion failed: " << error << '\n';
    return 1;
  }
  std::cout << "[Levye] Conversion completed (structural validation; build not run).\n";
  return 0;
}

}  // namespace Levye
