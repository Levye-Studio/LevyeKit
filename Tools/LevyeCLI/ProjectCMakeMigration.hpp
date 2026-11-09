#pragma once
#include <filesystem>
#include <string>

namespace Levye {
struct CMakeMigrationPlan {
  std::string rootCMake;
  std::string sourceManifest;
  bool updateRoot = false;
  bool createManifest = false;
  bool updateManifest = false;
};

class ProjectCMakeMigration {
 public:
  static bool Prepare(const std::filesystem::path& projectRoot, CMakeMigrationPlan& plan,
                      std::string& error);
  static bool EnsureSourceManifest(const std::filesystem::path& projectRoot, std::string& error);
};
}  // namespace Levye
