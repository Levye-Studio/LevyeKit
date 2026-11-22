
#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Levye {

/** @brief Selects the upgrade or conversion migration workflow. */
enum class ProjectMigrationMode { Upgrade, Convert };

/** @brief File operation planned by a project migration. */
enum class MigrationAction { Create, Modify, Move };

/** @brief One source or destination change in a migration plan. */
struct MigrationChange {
  MigrationAction action;
  std::filesystem::path source;
  std::filesystem::path destination;
  std::string description;
};

/** @brief Planned changes and validation diagnostics for a project migration. */
struct ProjectMigrationPlan {
  ProjectMigrationMode mode;

  std::filesystem::path projectRoot;
  std::filesystem::path templateRoot;

  std::vector<MigrationChange> changes;
  std::vector<std::string> warnings;
  std::vector<std::string> errors;

  /** @brief Reports whether the plan has no blocking errors. */
  bool CanApply() const { return errors.empty(); }
};

/** @brief Analyzes upgrade and conversion migrations. */
class ProjectMigration {
 public:
  /** @brief Builds a migration plan without applying it. */
  static ProjectMigrationPlan Analyze(const std::filesystem::path& projectRoot,
                                      const std::filesystem::path& templateRoot,
                                      ProjectMigrationMode mode);
};

}  // namespace Levye
