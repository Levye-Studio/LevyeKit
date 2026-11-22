#pragma once
#include <filesystem>
#include <string>

namespace Levye {
/** @brief Proposed root and source-manifest CMake changes. */
struct CMakeMigrationPlan {
  std::string rootCMake;
  std::string sourceManifest;
  bool updateRoot = false;
  bool createManifest = false;
  bool updateManifest = false;
};

/** @brief Prepares safe CMake migrations for older generated projects. */
class ProjectCMakeMigration {
 public:
  /** @brief Plans CMake edits for a supported project layout. */
  static bool Prepare(const std::filesystem::path& projectRoot, CMakeMigrationPlan& plan,
                      std::string& error);
  /** @brief Ensures the generated source manifest can be used by CMake. */
  static bool EnsureSourceManifest(const std::filesystem::path& projectRoot, std::string& error);
};
}  // namespace Levye
