#include "ProjectCMakeMigration.hpp"
#include <fstream>
#include <regex>
#include <sstream>
#include <string>

namespace Levye {
namespace {
namespace fs = std::filesystem;

bool ReadText(const fs::path& path, std::string& result) {
  std::ifstream input(path, std::ios::binary);
  if (!input) return false;
  std::ostringstream stream;
  stream << input.rdbuf();
  if (input.bad()) return false;
  result = stream.str();
  return true;
}

bool SafeRegularOrMissing(const fs::path& path, bool& exists) {
  std::error_code ec;
  const auto status = fs::symlink_status(path, ec);
  if (ec == std::errc::no_such_file_or_directory) {
    exists = false;
    return true;
  }
  if (ec) return false;
  exists = fs::exists(status);
  return !exists || fs::is_regular_file(status);
}
}  // namespace

bool ProjectCMakeMigration::Prepare(const fs::path& projectRoot,
                                    CMakeMigrationPlan& plan,
                                    std::string& error) {
  plan = {};
  error.clear();
  const fs::path root = projectRoot / "CMakeLists.txt";
  const fs::path manifest = projectRoot / "Source" / "CMakeLists.txt";
  bool rootExists = false;
  if (!SafeRegularOrMissing(root, rootExists) || !rootExists ||
      !ReadText(root, plan.rootCMake)) {
    error = "Cannot safely read project CMakeLists.txt.";
    return false;
  }

  const std::regex includePattern(
      R"(include\s*\(\s*"\$\{CMAKE_CURRENT_SOURCE_DIR\}/Source/CMakeLists\.txt"\s+OPTIONAL\s*\))");
  if (!std::regex_search(plan.rootCMake, includePattern)) {
    const std::string legacy =
        "add_library(\n    Game\n    SHARED\n    Source/Game.cpp\n)";
    const auto at = plan.rootCMake.find(legacy);
    if (at == std::string::npos ||
        plan.rootCMake.find(legacy, at + legacy.size()) != std::string::npos) {
      error = "Unrecognized Game CMake configuration; migration refused.";
      return false;
    }
    const std::string replacement =
        "set(LEVYE_GAME_SOURCES\n"
        "    \"${CMAKE_CURRENT_SOURCE_DIR}/Source/Game.cpp\"\n"
        ")\n\n"
        "include(\n"
        "    \"${CMAKE_CURRENT_SOURCE_DIR}/Source/CMakeLists.txt\"\n"
        "    OPTIONAL\n"
        ")\n\n"
        "add_library(\n    Game\n    SHARED\n    ${LEVYE_GAME_SOURCES}\n)";
    plan.rootCMake.replace(at, legacy.size(), replacement);
    plan.updateRoot = true;
  }

  bool exists = false;
  if (!SafeRegularOrMissing(manifest, exists)) {
    error = "Source manifest is not a regular file.";
    return false;
  }
  if (exists) {
    if (!ReadText(manifest, plan.sourceManifest)) {
      error = "Cannot read source manifest.";
      return false;
    }
  } else {
    plan.sourceManifest =
        "# Additional game sources managed by Levye CLI.\n\n"
        "# LEVYE_GENERATED_SOURCES_BEGIN\n"
        "list(APPEND LEVYE_GAME_SOURCES\n"
        ")\n"
        "# LEVYE_GENERATED_SOURCES_END\n";
    plan.createManifest = true;
  }
  return true;
}

// Compatibility entry point. New generators use Prepare + FileTransaction.
bool ProjectCMakeMigration::EnsureSourceManifest(const fs::path& projectRoot,
                                                 std::string& error) {
  CMakeMigrationPlan plan;
  if (!Prepare(projectRoot, plan, error)) return false;
  if (plan.updateRoot || plan.createManifest) {
    error = "Use the transactional generator to apply CMake migration.";
    return false;
  }
  return true;
}
}  // namespace Levye
