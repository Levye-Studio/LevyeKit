
#include "ProjectCMakeMigration.hpp"

#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

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

bool SafeSourcePath(const fs::path& sourceRoot, const fs::path& relative) {
  fs::path current = sourceRoot;
  std::error_code ec;
  if (!fs::is_directory(fs::symlink_status(current, ec)) || ec) return false;
  for (const auto& part : relative) {
    current /= part;
    ec.clear();
    const auto status = fs::symlink_status(current, ec);
    if (ec || fs::is_symlink(status)) return false;
    if (current == sourceRoot / relative) return fs::is_regular_file(status);
    if (!fs::is_directory(status)) return false;
  }
  return false;
}

// Locate a CMake command while allowing multiline arguments.
// This first version supports literal arguments and comments,
// but deliberately rejects nested expressions.
bool FindGameLibrary(const std::string& content, std::size_t& begin, std::size_t& end,
                     std::vector<std::string>& sources) {
  const std::regex commandPattern(R"(^[ \t]*add_library\s*\()",
                                  std::regex::icase | std::regex_constants::multiline);

  auto search = content.cbegin();
  bool found = false;

  while (search != content.cend()) {
    std::smatch match;
    if (!std::regex_search(search, content.cend(), match, commandPattern)) {
      break;
    }

    const auto start = static_cast<std::size_t>(std::distance(content.cbegin(), search)) +
                       static_cast<std::size_t>(match.position());

    const auto open = start + static_cast<std::size_t>(match.length()) - 1;

    bool quoted = false;
    bool comment = false;
    bool escaped = false;
    int depth = 1;
    std::size_t close = std::string::npos;

    for (std::size_t i = open + 1; i < content.size(); ++i) {
      const char c = content[i];

      if (comment) {
        if (c == '\n') comment = false;
        continue;
      }

      if (escaped) {
        escaped = false;
        continue;
      }

      if (c == '\\') {
        escaped = true;
        continue;
      }

      if (c == '"') {
        quoted = !quoted;
        continue;
      }

      if (!quoted && c == '#') {
        comment = true;
        continue;
      }

      if (!quoted && c == '(') ++depth;

      if (!quoted && c == ')') {
        --depth;
        if (depth == 0) {
          close = i;
          break;
        }
      }
    }

    if (close == std::string::npos || quoted || depth != 0) return false;

    const auto body = content.substr(open + 1, close - open - 1);

    std::istringstream tokens(body);
    std::string target;
    std::string kind;

    tokens >> target >> kind;

    if (target == "Game") {
      if (found || kind != "SHARED") return false;
      // Only accept a literal source list for the Game target.
      if (body.find('#') != std::string::npos || body.find('"') != std::string::npos ||
          body.find('(') != std::string::npos || body.find(')') != std::string::npos) {
        return false;
      }
      found = true;
      begin = start;
      end = close + 1;

      std::string source;
      while (tokens >> source) {
        if (source.find('$') != std::string::npos || source.find(';') != std::string::npos ||
            source.find('\\') != std::string::npos || fs::path(source).is_absolute() ||
            source.find("..") != std::string::npos || !source.starts_with("Source/") ||
            !source.ends_with(".cpp") || source == "Source/Main.cpp") {
          return false;
        }

        sources.push_back(source.substr(7));
      }

      if (sources.empty()) return false;
    }

    search = content.cbegin() + static_cast<std::ptrdiff_t>(close + 1);
  }

  return found;
}

std::string MakeManifest(const std::vector<std::string>& sources) {
  std::ostringstream out;

  out << "# LevyeKit game source manifest.\n\n";
  out << "set(LEVYE_GAME_SOURCES\n";

  for (const auto& source : sources) {
    out << "    \"${CMAKE_CURRENT_LIST_DIR}/" << source << "\"\n";
  }

  out << ")\n\n";
  out << "# LEVYE_GENERATED_SOURCES_BEGIN\n";
  out << "list(APPEND LEVYE_GAME_SOURCES\n";
  out << ")\n";
  out << "# LEVYE_GENERATED_SOURCES_END\n";

  return out.str();
}

}  // namespace

bool ProjectCMakeMigration::Prepare(const fs::path& projectRoot, CMakeMigrationPlan& plan,
                                    std::string& error) {
  plan = {};
  error.clear();

  const auto root = projectRoot / "CMakeLists.txt";
  const auto manifest = projectRoot / "Source" / "CMakeLists.txt";

  bool rootExists = false;

  if (!SafeRegularOrMissing(root, rootExists) || !rootExists || !ReadText(root, plan.rootCMake)) {
    error = "Cannot safely read project CMakeLists.txt.";
    return false;
  }

  bool manifestExists = false;

  if (!SafeRegularOrMissing(manifest, manifestExists)) {
    error = "Source manifest is not a regular file.";
    return false;
  }

  const std::regex includePattern(
      R"(^[ \t]*include\s*\(\s*"\$\{CMAKE_CURRENT_SOURCE_DIR\}/Source/CMakeLists\.txt"\s+OPTIONAL\s*\))",
      std::regex::icase | std::regex_constants::multiline);

  const bool integrated = std::regex_search(plan.rootCMake, includePattern);

  if (integrated) {
    const std::regex gameSources(
        R"(^[ \t]*add_library\s*\(\s*Game\s+SHARED\s+\$\{LEVYE_GAME_SOURCES\}\s*\))",
        std::regex::icase | std::regex_constants::multiline);
    if (!std::regex_search(plan.rootCMake, gameSources)) {
      error = "Source manifest include exists, but Game source integration is missing.";
      return false;
    }

    if (manifestExists) {
      if (!ReadText(manifest, plan.sourceManifest)) {
        error = "Cannot read existing source manifest.";
        return false;
      }
      constexpr std::string_view begin = "# LEVYE_GENERATED_SOURCES_BEGIN";
      constexpr std::string_view end = "# LEVYE_GENERATED_SOURCES_END";
      const auto b = plan.sourceManifest.find(begin);
      const auto e = plan.sourceManifest.find(end);
      if ((b == std::string::npos) != (e == std::string::npos) ||
          (b != std::string::npos &&
           (b >= e || plan.sourceManifest.find(begin, b + begin.size()) != std::string::npos ||
            plan.sourceManifest.find(end, e + end.size()) != std::string::npos))) {
        error = "Existing source manifest has invalid generator markers.";
        return false;
      }
      if (b == std::string::npos) {
        if (plan.sourceManifest.find("LEVYE_GAME_SOURCES") == std::string::npos) {
          error = "Existing source manifest does not define game sources.";
          return false;
        }
        if (!plan.sourceManifest.empty() && plan.sourceManifest.back() != '\n')
          plan.sourceManifest += '\n';
        plan.sourceManifest +=
            "\n# LEVYE_GENERATED_SOURCES_BEGIN\n"
            "list(APPEND LEVYE_GAME_SOURCES\n"
            ")\n"
            "# LEVYE_GENERATED_SOURCES_END\n";
        plan.updateManifest = true;
      }
    } else {
      const std::regex rootSources(R"(^[ \t]*set\s*\(\s*LEVYE_GAME_SOURCES\s+[^)]*\))",
                                   std::regex::icase | std::regex_constants::multiline);
      if (!std::regex_search(plan.rootCMake, rootSources)) {
        error = "Missing source manifest and no game sources in the root CMake file.";
        return false;
      }
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

  // Do not overwrite a manifest that the upgrader
  // does not yet know how to merge safely.
  if (manifestExists) {
    error = "Existing source manifest requires manual migration.";
    return false;
  }

  std::size_t begin = 0;
  std::size_t end = 0;
  std::vector<std::string> sources;

  if (!FindGameLibrary(plan.rootCMake, begin, end, sources)) {
    error = "Unsupported or ambiguous Game target source list.";
    return false;
  }

  // Ensure every migrated source actually exists.
  for (const auto& source : sources) {
    if (!SafeSourcePath(projectRoot / "Source", fs::path(source))) {
      error = "Game source is missing or unsafe: " + source;
      return false;
    }
  }

  plan.sourceManifest = MakeManifest(sources);
  plan.createManifest = true;

  const std::string replacement =
      "include(\n"
      "    \"${CMAKE_CURRENT_SOURCE_DIR}/Source/CMakeLists.txt\"\n"
      "    OPTIONAL\n"
      ")\n\n"
      "add_library(\n"
      "    Game\n"
      "    SHARED\n"
      "    ${LEVYE_GAME_SOURCES}\n"
      ")";

  plan.rootCMake.replace(begin, end - begin, replacement);

  plan.updateRoot = true;

  return true;
}

bool ProjectCMakeMigration::EnsureSourceManifest(const fs::path& projectRoot, std::string& error) {
  CMakeMigrationPlan plan;

  if (!Prepare(projectRoot, plan, error)) return false;

  if (plan.updateRoot || plan.createManifest || plan.updateManifest) {
    error = "Use the transactional migration to apply CMake changes.";
    return false;
  }

  return true;
}

}  // namespace Levye
