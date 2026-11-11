
#include "ProjectModuleMigration.hpp"

#include <regex>
#include <string>

namespace Levye {
namespace {

const std::string kHelper = "include(\"${LEVYE_ROOT}/cmake/LevyeProjectModules.cmake\")";

const std::string kMessages =
    "\nmessage(STATUS \"LevyeKit serialization: ${LEVYE_WITH_SERIALIZATION}\")\n"
    "message(STATUS \"LevyeKit ImGui: ${LEVYE_WITH_IMGUI}\")\n"
    "message(STATUS \"LevyeKit Animation: ${LEVYE_WITH_ANIMATION}\")\n"
    "message(STATUS \"LevyeKit Physics: ${LEVYE_WITH_PHYSICS}\")\n";

// Recognize the legacy configuration block using its section
// boundaries, rather than relying on exact indentation.
const std::regex kLegacySection(
    R"(^[ \t]*# -+\r?\n[ \t]*# Optional LevyeKit modules\r?\n[ \t]*# -+\r?\n[\s\S]*?(?=^[ \t]*add_subdirectory\s*\())",
    std::regex::icase | std::regex_constants::multiline);

const std::regex kModuleHelper(
    R"(^[ \t]*include\s*\(\s*"?\$\{LEVYE_ROOT\}/cmake/LevyeProjectModules\.cmake"?\s*\))",
    std::regex::icase | std::regex_constants::multiline);

}  // namespace

bool ProjectModuleMigration::Prepare(std::string& cmake, bool& changed, std::string& error) {
  changed = false;
  error.clear();

  if (!std::regex_search(cmake, kModuleHelper)) {
    std::smatch match;
    if (std::regex_search(cmake, match, kLegacySection)) {
      const std::string legacy = match.str();
      const bool knownLegacy = legacy.find("LEVYE_SERIALIZATION_SETTING") != std::string::npos &&
                               legacy.find("LEVYE_IMGUI_SETTING") != std::string::npos &&
                               legacy.find("LEVYE_WITH_SERIALIZATION") != std::string::npos &&
                               legacy.find("LEVYE_WITH_IMGUI") != std::string::npos &&
                               legacy.find("CMAKE_CONFIGURE_DEPENDS") != std::string::npos &&
                               legacy.find("LEVYE_WITH_ANIMATION") == std::string::npos &&
                               legacy.find("LEVYE_WITH_PHYSICS") == std::string::npos;
      if (!knownLegacy && legacy.find("LEVYE_WITH_") == std::string::npos &&
          legacy.find("file(") == std::string::npos) {
        cmake.insert(static_cast<std::size_t>(match.position() + match.length()),
                     kHelper + "\n" + kMessages + "\n");
        changed = true;
      } else if (!knownLegacy) {
        error = "Legacy module section contains an unsupported layout.";
        return false;
      } else {
        const std::regex assignment(R"(\bset\s*\(\s*([A-Za-z_][A-Za-z0-9_]*))", std::regex::icase);
        for (auto it = std::sregex_iterator(legacy.begin(), legacy.end(), assignment);
             it != std::sregex_iterator(); ++it) {
          const auto variable = (*it)[1].str();
          if (variable != "LEVYE_WITH_SERIALIZATION" && variable != "LEVYE_WITH_IMGUI") {
            error = "Custom module assignment requires manual migration: " + variable;
            return false;
          }
        }
        const std::regex command(R"((^|\n)[ \t]*([A-Za-z_]+)[ \t]*\()", std::regex::icase);
        int fileCount = 0;
        int messageCount = 0;
        int propertyCount = 0;
        for (auto it = std::sregex_iterator(legacy.begin(), legacy.end(), command);
             it != std::sregex_iterator(); ++it) {
          const auto name = (*it)[2].str();
          if (name == "file")
            ++fileCount;
          else if (name == "message")
            ++messageCount;
          else if (name == "set_property")
            ++propertyCount;
          else if (name != "set" && name != "if" && name != "endif") {
            error = "Custom module command requires manual migration: " + name;
            return false;
          }
        }
        if (fileCount != 2 || messageCount != 2 || propertyCount != 1) {
          error = "Legacy module section contains custom commands.";
          return false;
        }
        const std::string replacement =
            "# ---------------------------------------------------------\n"
            "# Optional LevyeKit modules\n"
            "# ---------------------------------------------------------\n\n" +
            kHelper + "\n" + kMessages + "\n";
        cmake.replace(static_cast<std::size_t>(match.position()),
                      static_cast<std::size_t>(match.length()), replacement);
        changed = true;
      }
    } else {
      const std::regex subdirectory(R"(^[ \t]*add_subdirectory\s*\(\s*"?\$\{LEVYE_ROOT\})",
                                    std::regex::icase | std::regex_constants::multiline);
      std::smatch subMatch;
      if (!std::regex_search(cmake, subMatch, subdirectory)) {
        error = "Cannot locate the LevyeKit CMake subdirectory.";
        return false;
      }
      const auto at = static_cast<std::size_t>(subMatch.position());
      const auto beforeFramework = cmake.substr(0, at);
      if (beforeFramework.find("LEVYE_WITH_") != std::string::npos) {
        error = "Unrecognized module configuration requires manual migration.";
        return false;
      }
      cmake.insert(at, kHelper + "\n" + kMessages + "\n");
      changed = true;
    }
  }

  // Older templates did not forward every enabled module to the game DLL.
  // Add only the missing definitions; leave custom link rules untouched.
  for (const char* module : {"SERIALIZATION", "IMGUI", "ANIMATION", "PHYSICS"}) {
    const std::string variable = std::string("LEVYE_WITH_") + module;
    const std::regex definition(
        "target_compile_definitions\\s*\\(\\s*Game\\b[^)]*\\b" + variable + "=1\\b",
        std::regex::icase);
    if (std::regex_search(cmake, definition)) continue;
    cmake += "\nif(" + variable +
             ")\n"
             "    target_compile_definitions(Game PRIVATE " +
             variable +
             "=1)\n"
             "endif()\n";
    changed = true;
  }
  return true;
}

}  // namespace Levye
