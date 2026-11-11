
#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Levye {

enum class ProjectMigrationMode { Upgrade, Convert };

enum class MigrationAction { Create, Modify, Move };

struct MigrationChange {
  MigrationAction action;
  std::filesystem::path source;
  std::filesystem::path destination;
  std::string description;
};

struct ProjectMigrationPlan {
  ProjectMigrationMode mode;

  std::filesystem::path projectRoot;
  std::filesystem::path templateRoot;

  std::vector<MigrationChange> changes;
  std::vector<std::string> warnings;
  std::vector<std::string> errors;

  bool CanApply() const { return errors.empty(); }
};

class ProjectMigration {
 public:
  static ProjectMigrationPlan Analyze(const std::filesystem::path& projectRoot,
                                      const std::filesystem::path& templateRoot,
                                      ProjectMigrationMode mode);
};

}  // namespace Levye
