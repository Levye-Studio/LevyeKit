
#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Levye {

/** @brief CMake target kind recognized by the project converter. */
enum class ConversionProjectType { Executable, StaticLibrary, SharedLibrary, Unsupported };

/** @brief Detected application entry-point and lifecycle shape. */
enum class ConversionLifecycle {
  NoEntryPoint,
  StandardMain,
  RaylibApplication,
  SDLApplication,
  GLFWApplication,
  Unknown
};

/** @brief A read-only capability or migration blocker found in the source tree. */
struct ConversionFinding {
  std::string category;
  std::string detail;
  bool requiresAdapter = false;
};

/** @brief Source and build properties collected for one convertible target. */
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

/** @brief Read-only analysis used to plan project conversion. */
struct ConversionAnalysis {
  std::filesystem::path projectRoot;
  std::vector<ConversionTarget> targets;
  std::vector<std::string> warnings;
  std::vector<std::string> errors;
  std::vector<ConversionFinding> findings;
  std::vector<std::filesystem::path> observedSources;
  std::vector<std::string> observedTargets;
  std::vector<std::string> observedDependencies;

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

  /** @brief Reports whether analysis found conversion-blocking errors. */
  bool CanConvert() const {
    return errors.empty();
  }
};

/** @brief Inspects a CMake project without changing its files. */
class ProjectAnalyzer {
 public:
  /** @brief Analyzes a project root for supported conversion patterns. */
  static ConversionAnalysis Analyze(const std::filesystem::path& projectRoot,
                                    const std::filesystem::path& assetOverride = {});
};

}  // namespace Levye
