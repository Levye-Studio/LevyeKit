
#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Levye {

enum class ConversionProjectType { Executable, StaticLibrary, SharedLibrary, Unsupported };

enum class ConversionLifecycle {
  NoEntryPoint,
  StandardMain,
  RaylibApplication,
  SDLApplication,
  GLFWApplication,
  Unknown
};

struct ConversionTarget {
  std::string name;
  ConversionProjectType type = ConversionProjectType::Unsupported;

  std::vector<std::filesystem::path> sources;
  std::vector<std::string> dependencies;
  std::vector<std::string> includeDirectories;
  std::vector<std::string> compileDefinitions;
  std::vector<std::string> compileOptions;
  std::vector<std::string> cmakeSettings;
};

struct ConversionAnalysis {
  std::filesystem::path projectRoot;
  std::vector<ConversionTarget> targets;
  std::vector<std::string> warnings;
  std::vector<std::string> errors;

  ConversionLifecycle lifecycle = ConversionLifecycle::Unknown;

  std::filesystem::path entryPoint;
  std::filesystem::path assetDirectory = "Assets";
  std::string projectName;
  std::string originalCMake;
  std::string drawCalls;
  int windowWidth = 0;
  int windowHeight = 0;
  int targetFPS = 0;
  bool alreadyLevyeKit = false;

  bool CanConvert() const { return errors.empty(); }
};

class ProjectAnalyzer {
 public:
  static ConversionAnalysis Analyze(const std::filesystem::path& projectRoot);
};

}  // namespace Levye
