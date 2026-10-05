#include "AddCommand.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <toml++/toml.hpp>

#include "ModuleRegistry.hpp"

namespace Levye {

bool AddCommand::Execute(const std::filesystem::path& projectDirectory,
                         std::string_view module) {
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
          std::cout << "[Levye] Module '" << module
                    << "' is already enabled.\n";
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
  const std::regex sectionPattern(
      R"((^|\n)[ \t]*\[modules\][ \t]*(?:\r?\n|$))");

  std::smatch section;

  if (!std::regex_search(content, section, sectionPattern)) {
    if (!content.empty() && content.back() != '\n') content += '\n';

    content += "\n[modules]\n";
    content += std::string(module) + " = true\n";
  } else {
    const std::size_t sectionEnd =
        static_cast<std::size_t>(section.position() + section.length());

    const std::size_t nextSection = content.find("\n[", sectionEnd);

    const std::size_t end =
        nextSection == std::string::npos ? content.size() : nextSection + 1;

    std::string body = content.substr(sectionEnd, end - sectionEnd);

    const std::regex settingPattern(
        "(^|\\n)[ \\t]*" + std::string(module) +
        R"([ \t]*=[ \t]*(true|false)[ \t]*(?=\r?\n|$))");

    if (std::regex_search(body, settingPattern)) {
      body = std::regex_replace(body, settingPattern,
                                "$1" + std::string(module) + " = true",
                                std::regex_constants::format_first_only);
    } else {
      if (!body.empty() && body.back() != '\n') body += '\n';

      body += std::string(module) + " = true\n";
    }

    content.replace(sectionEnd, end - sectionEnd, body);
  }

  // Validate the resulting TOML before writing it.
  try {
    toml::parse(content);
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
  // Note: std::filesystem::rename can fail on Windows if the
  // destination already exists; handle that case in the next pass.
  std::error_code error;
  std::filesystem::rename(temporaryPath, path, error);

  if (error) {
    std::filesystem::remove(temporaryPath);

    std::cerr << "[Levye] Cannot replace configuration: " << error.message()
              << '\n';
    return false;
  }

  std::cout << "[Levye] Added module: " << module << '\n';
  return true;
}

}  // namespace Levye