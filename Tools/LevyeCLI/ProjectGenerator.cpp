#include "ProjectGenerator.hpp"

#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <toml++/toml.hpp>

#include "ModuleRegistry.hpp"

namespace {

void ReplaceAll(std::string& content, const std::string& token, const std::string& value) {
  std::size_t position = 0;

  while ((position = content.find(token, position)) != std::string::npos) {
    content.replace(position, token.length(), value);

    position += value.length();
  }
}

}  // namespace

namespace Levye {

bool ProjectGenerator::Generate(const std::string& name,
                                const std::filesystem::path& outputDirectory,
                                const std::filesystem::path& templateDirectory,
                                const std::filesystem::path& frameworkDirectory,
                                const std::vector<std::string>& modules) {
  if (name.empty()) {
    std::cerr << "[Levye] Project name cannot be empty.\n";

    return false;
  }

  const std::string target = CreateTargetName(name);

  for (const auto& module : modules) {
    if (!ModuleRegistry::Exists(module)) {
      std::cerr << "[Levye] Unknown module: " << module << '\n';
      return false;
    }
  }

  if (target.empty()) {
    std::cerr << "[Levye] Project name does not contain "
              << "a valid target name.\n";

    return false;
  }

  if (!std::filesystem::exists(templateDirectory)) {
    std::cerr << "[Levye] Project template does not exist: " << templateDirectory << '\n';

    return false;
  }

  const std::filesystem::path projectDirectory = outputDirectory / target;

  if (std::filesystem::exists(projectDirectory)) {
    std::cerr << "[Levye] Directory already exists: " << projectDirectory << '\n';

    return false;
  }

  std::error_code error;

  std::filesystem::create_directories(projectDirectory, error);

  if (error) {
    std::cerr << "[Levye] Failed to create project directory: " << error.message() << '\n';

    return false;
  }

  std::filesystem::copy(templateDirectory, projectDirectory,
                        std::filesystem::copy_options::recursive, error);

  if (error) {
    std::cerr << "[Levye] Failed to copy project template: " << error.message() << '\n';

    std::filesystem::remove_all(projectDirectory);

    return false;
  }

  const std::filesystem::path filesToProcess[] = {projectDirectory / "levye.project",
                                                  projectDirectory / "CMakeLists.txt",
                                                  projectDirectory / "Source/Game.cpp"};

  for (const auto& path : filesToProcess) {
    if (!ProcessTemplateFile(path, name, target, frameworkDirectory)) {
      std::cerr << "[Levye] Failed to process template file: " << path << '\n';

      std::filesystem::remove_all(projectDirectory);

      return false;
    }
  }

  // Enable optional modules requested during project creation.
  if (!modules.empty()) {
    const auto configPath = projectDirectory / "levye.project";

    std::ifstream input(configPath, std::ios::binary);

    if (!input) {
      std::cerr << "[Levye] Cannot read project configuration.\n";
      std::filesystem::remove_all(projectDirectory);
      return false;
    }

    std::string content{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};

    if (input.bad()) {
      std::cerr << "[Levye] Failed reading project configuration.\n";
      std::filesystem::remove_all(projectDirectory);
      return false;
    }

    input.close();

    // Validate the original configuration before modifying it.
    try {
      const auto config = toml::parse(content);

      const auto* moduleTable = config["modules"].as_table();

      if (!moduleTable) {
        std::cerr << "[Levye] Missing [modules] section.\n";
        std::filesystem::remove_all(projectDirectory);
        return false;
      }

      for (const auto& module : modules) {
        const auto* setting = moduleTable->get(module);

        if (!setting || !setting->is_boolean()) {
          std::cerr << "[Levye] Missing or invalid module setting: " << module << '\n';

          std::filesystem::remove_all(projectDirectory);
          return false;
        }
      }
    } catch (const toml::parse_error& error) {
      std::cerr << "[Levye] Invalid project configuration: " << error << '\n';

      std::filesystem::remove_all(projectDirectory);
      return false;
    }

    // Locate the [modules] section, including optional inline comments.
    const std::regex sectionPattern(R"((^|\n)[ \t]*\[modules\][ \t]*(?:#.*)?(?:\r?\n|$))");

    std::smatch section;

    if (!std::regex_search(content, section, sectionPattern)) {
      std::cerr << "[Levye] Cannot locate [modules] section.\n";
      std::filesystem::remove_all(projectDirectory);
      return false;
    }

    const std::size_t sectionEnd = static_cast<std::size_t>(section.position() + section.length());

    // Find the next TOML section without touching unrelated settings.
    const std::regex tablePattern(
        R"(^[ \t]*(?:\[\[[^\]\r\n]+\]\]|\[[^\]\r\n]+\])[ \t]*(?:#.*)?\r?$)");

    std::size_t sectionLimit = content.size();

    for (std::size_t lineStart = sectionEnd; lineStart < content.size();) {
      const auto lineEnd = content.find('\n', lineStart);
      const auto line = content.substr(lineStart, lineEnd - lineStart);

      if (std::regex_match(line, tablePattern)) {
        sectionLimit = lineStart;
        break;
      }

      if (lineEnd == std::string::npos) break;
      lineStart = lineEnd + 1;
    }

    std::string body = content.substr(sectionEnd, sectionLimit - sectionEnd);

    for (const auto& module : modules) {
      const std::regex settingPattern("(^|\\n)([ \\t]*" + module +
                                      R"([ \t]*=[ \t]*)(true|false)([ \t]*(?:#.*)?(?=\r?\n|$)))");

      std::smatch setting;

      if (!std::regex_search(body, setting, settingPattern)) {
        std::cerr << "[Levye] Cannot locate module setting: " << module << '\n';

        std::filesystem::remove_all(projectDirectory);
        return false;
      }

      // Replace only the boolean value, preserving formatting and comments.
      body.replace(static_cast<std::size_t>(setting.position(3)),
                   static_cast<std::size_t>(setting.length(3)), "true");
    }

    content.replace(sectionEnd, sectionLimit - sectionEnd, body);

    // Verify that the resulting TOML enables every requested module.
    try {
      const auto config = toml::parse(content);
      const auto* moduleTable = config["modules"].as_table();

      if (!moduleTable) {
        std::cerr << "[Levye] Updated module table is missing.\n";
        std::filesystem::remove_all(projectDirectory);
        return false;
      }

      for (const auto& module : modules) {
        const auto* setting = moduleTable->get(module);

        if (!setting || setting->value<bool>() != true) {
          std::cerr << "[Levye] Failed to enable module: " << module << '\n';

          std::filesystem::remove_all(projectDirectory);
          return false;
        }
      }
    } catch (const toml::parse_error& error) {
      std::cerr << "[Levye] Updated configuration is invalid: " << error << '\n';

      std::filesystem::remove_all(projectDirectory);
      return false;
    }

    std::ofstream output(configPath, std::ios::binary | std::ios::trunc);

    if (!output) {
      std::cerr << "[Levye] Cannot write project configuration.\n";
      std::filesystem::remove_all(projectDirectory);
      return false;
    }

    output << content;
    output.close();

    if (!output) {
      std::cerr << "[Levye] Failed writing project configuration.\n";
      std::filesystem::remove_all(projectDirectory);
      return false;
    }
  }

  std::cout << "[Levye] Created project \"" << name << "\" at " << projectDirectory << '\n';

  std::cout << "\n[Levye] Next steps:\n"
            << "  cd \"" << projectDirectory.string() << "\"\n"
            << "  levye build\n"
            << "  levye run\n";

  return true;
}

std::string ProjectGenerator::CreateTargetName(const std::string& name) {
  std::string target;

  target.reserve(name.size());

  for (const unsigned char character : name) {
    if (std::isalnum(character) || character == '_') {
      target += static_cast<char>(character);
    }
  }

  return target;
}

bool ProjectGenerator::ProcessTemplateFile(const std::filesystem::path& path,
                                           const std::string& projectName,
                                           const std::string& projectTarget,
                                           const std::filesystem::path& frameworkDirectory) {
  std::ifstream input(path);

  if (!input) return false;

  std::string content{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};

  input.close();

  /*
   * Template values are replaced only in files explicitly processed by the
   * generator. Binary assets copied from the template are never modified.
   */
  ReplaceAll(content, "{{PROJECT_NAME}}", projectName);

  ReplaceAll(content, "{{PROJECT_TARGET}}", projectTarget);

  ReplaceAll(content, "{{LEVYE_ROOT}}", frameworkDirectory.generic_string());

  std::ofstream output(path, std::ios::trunc);

  if (!output) return false;

  output << content;

  return output.good();
}

}  // namespace Levye
