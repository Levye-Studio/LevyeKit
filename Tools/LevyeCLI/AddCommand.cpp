#include "AddCommand.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <system_error>
#include <toml++/toml.hpp>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include "ModuleRegistry.hpp"

namespace Levye {

bool AddCommand::Execute(const std::filesystem::path& projectDirectory, std::string_view module) {
  if (!ModuleRegistry::Exists(module)) {
    std::cerr << "[Levye] Unknown module: " << module << '\n';
    return false;
  }

  const auto path = projectDirectory / "levye.project";

  // Validate the TOML before modifying anything.
  try {
    const auto config = toml::parse_file(path.string());

    if (!config["project"].as_table()) {
      std::cerr << "[Levye] Invalid project configuration.\n";
      return false;
    }

    if (const auto* modules = config["modules"].as_table()) {
      if (const auto* setting = modules->get(module)) {
        if (!setting->is_boolean()) {
          std::cerr << "[Levye] Invalid module setting.\n";
          return false;
        }

        if (setting->value<bool>().value_or(false)) {
          std::cout << "[Levye] Module '" << module << "' is already enabled.\n";
          return true;
        }
      }
    }
  } catch (const toml::parse_error& error) {
    std::cerr << "[Levye] " << error << '\n';
    return false;
  }

  std::ifstream input(path);

  if (!input) {
    std::cerr << "[Levye] Cannot read levye.project.\n";
    return false;
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();

  if (!input.eof() && input.fail()) {
    std::cerr << "[Levye] Failed reading levye.project.\n";
    return false;
  }

  std::string content = buffer.str();

  // Find the [modules] section without changing other sections.
  const std::regex sectionPattern(R"((^|\n)[ \t]*\[modules\][ \t]*(?:#.*)?(?:\r?\n|$))");

  std::smatch section;

  if (!std::regex_search(content, section, sectionPattern)) {
    if (!content.empty() && content.back() != '\n') content += '\n';

    content += "\n[modules]\n";
    content += std::string(module) + " = true\n";
  } else {
    const std::size_t sectionEnd = static_cast<std::size_t>(section.position() + section.length());

    // TOML permits leading whitespace before another table header.
    const std::regex tablePattern(
        R"(^[ \t]*(?:\[\[[^\]\r\n]+\]\]|\[[^\]\r\n]+\])[ \t]*(?:#.*)?\r?$)");
    std::size_t end = content.size();
    for (std::size_t lineStart = sectionEnd; lineStart < content.size();) {
      const auto lineEnd = content.find('\n', lineStart);
      const std::string line = content.substr(lineStart, lineEnd - lineStart);
      if (std::regex_match(line, tablePattern)) {
        end = lineStart;
        break;
      }
      if (lineEnd == std::string::npos) break;
      lineStart = lineEnd + 1;
    }

    std::string body = content.substr(sectionEnd, end - sectionEnd);
    if (sectionEnd > 0 && content[sectionEnd - 1] != '\n') body.insert(body.begin(), '\n');

    const std::regex settingPattern("(^|\\n)[ \\t]*" + std::string(module) +
                                    R"([ \t]*=[ \t]*(true|false)[ \t]*(?:#.*)?(?=\r?\n|$))");

    std::smatch setting;
    if (std::regex_search(body, setting, settingPattern)) {
      body.replace(static_cast<std::size_t>(setting.position(2)), setting.length(2), "true");
    } else {
      if (!body.empty() && body.back() != '\n') body += '\n';

      body += std::string(module) + " = true\n";
    }

    content.replace(sectionEnd, end - sectionEnd, body);
  }

  // Validate the resulting TOML before writing it.
  try {
    (void)toml::parse(content);
  } catch (const toml::parse_error& error) {
    std::cerr << "[Levye] Updated configuration is invalid: " << error << '\n';
    return false;
  }

  // Write a temporary file first.
  const auto temporaryPath = std::filesystem::path(path.string() + ".tmp");

  {
    std::ofstream output(temporaryPath, std::ios::trunc);

    if (!output) {
      std::cerr << "[Levye] Cannot write project configuration.\n";
      return false;
    }

    output << content;
    output.close();

    if (!output) {
      std::filesystem::remove(temporaryPath);
      return false;
    }
  }

  // Replace the original only after the new file is complete.
  std::error_code error;
#if defined(_WIN32)
  if (!MoveFileExW(temporaryPath.wstring().c_str(), path.wstring().c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    error = std::error_code(static_cast<int>(GetLastError()), std::system_category());
  }
#else
  std::filesystem::rename(temporaryPath, path, error);
#endif

  if (error) {
    std::filesystem::remove(temporaryPath);

    std::cerr << "[Levye] Cannot replace configuration: " << error.message() << '\n';
    return false;
  }

  std::cout << "[Levye] Added module: " << module << '\n';
  return true;
}

}  // namespace Levye
