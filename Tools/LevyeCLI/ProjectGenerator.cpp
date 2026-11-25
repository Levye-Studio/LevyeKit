#include "ProjectGenerator.hpp"

#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <toml++/toml.hpp>
#include <utility>

#include "ModuleRegistry.hpp"

namespace {

std::string TomlString(std::string_view value) {
  constexpr char hex[] = "0123456789ABCDEF";
  std::string quoted = "\"";
  for (unsigned char ch : value) {
    switch (ch) {
      case '"':
        quoted += "\\\"";
        break;
      case '\\':
        quoted += "\\\\";
        break;
      case '\n':
        quoted += "\\n";
        break;
      case '\r':
        quoted += "\\r";
        break;
      case '\t':
        quoted += "\\t";
        break;
      default:
        if (ch < 0x20 || ch == 0x7f) {
          quoted += "\\u00";
          quoted += hex[ch >> 4];
          quoted += hex[ch & 0xf];
        } else {
          quoted += static_cast<char>(ch);
        }
    }
  }
  return quoted + '"';
}

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
                                const std::vector<std::string>& modules, bool android, bool ios,
                                bool web) {
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

  for (const char* directory : {"Textures", "Audio", "Fonts", "Shaders", "Models"}) {
    std::filesystem::create_directories(projectDirectory / "Assets" / directory, error);
    if (error) {
      std::cerr << "[Levye] Failed to create asset directory: " << error.message() << '\n';
      std::filesystem::remove_all(projectDirectory);
      return false;
    }
  }

  const std::filesystem::path filesToProcess[] = {projectDirectory / "levye.project",
                                                  projectDirectory / "CMakeLists.txt",
                                                  projectDirectory / "Source/Game.cpp"};

  for (const auto& path : filesToProcess) {
    if (!ProcessTemplateFile(path, name, target, frameworkDirectory,
                             android ? "android" : (ios ? "ios" : (web ? "web" : "desktop")))) {
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

      if (lineEnd == std::string::npos)
        break;
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

std::string ProjectGenerator::RenderText(std::string content, const std::string& projectName,
                                         const std::string& projectTarget,
                                         const std::filesystem::path& frameworkDirectory,
                                         std::string_view defaultPlatform) {
  // The name placeholder is used inside quoted TOML and C++ string literals.
  const std::string quotedName = TomlString(projectName);
  ReplaceAll(content, "{{PROJECT_NAME}}", quotedName.substr(1, quotedName.size() - 2));
  ReplaceAll(content, "{{PROJECT_TARGET}}", projectTarget);
  std::string packageSuffix = "game";
  for (unsigned char character : projectTarget) {
    if (std::isalnum(character))
      packageSuffix += static_cast<char>(std::tolower(character));
    else
      packageSuffix += '_';
  }
  const bool android = defaultPlatform == "android";
  ReplaceAll(content, "{{DISPLAY_CONFIG}}",
             std::string("[display]\norientation = \"") + (android ? "portrait" : "landscape") +
                 "\"\nlogical_width = " + (android ? "720" : "1280") +
                 "\nlogical_height = " + (android ? "1280" : "720") +
                 "\nscale_mode = \"fit\"\nfilter = \"linear\"\ntarget_fps = 60\nvsync = "
                 "false\nfullscreen = " +
                 (defaultPlatform == "desktop" ? "false" : "true") + "\nsafe_area = \"inset\"\n");
  ReplaceAll(content, "{{DESKTOP_CONFIG}}",
             defaultPlatform == "desktop"
                 ? "[desktop]\nwindow_width = 1280\nwindow_height = 720\nresizable = true\n"
                 : "");

  std::string androidConfig;

  if (android) {
    androidConfig =
        "[android]\n"
        "package = \"com.levyekit." +
        packageSuffix +
        "\"\n"
        "app_name = " +
        TomlString(projectName) +
        "\n"
        "version_name = \"1.0\"\n"
        "version_code = 1\n"
        "# icon = \"Icons/launcher.png\"\n"
        "# adaptive_icon_foreground = \"Icons/foreground.png\"\n"
        "# adaptive_icon_background = \"#202A44\"\n";
  }

  ReplaceAll(content, "{{ANDROID_CONFIG}}", androidConfig);
  std::string iosConfig;
  if (defaultPlatform == "ios") {
    iosConfig =
        "[ios]\n"
        "bundle_id = \"com.levyekit." +
        packageSuffix +
        "\"\n"
        "deployment_target = \"15.0\"\n"
        "display_name = " +
        TomlString(projectName) +
        "\n"
        "version_name = \"1.0.0\"\n"
        "version_code = 1\n";
  }
  ReplaceAll(content, "{{IOS_CONFIG}}", iosConfig);
  ReplaceAll(content, "{{LEVYE_ROOT}}", frameworkDirectory.generic_string());
  ReplaceAll(content, "{{DEFAULT_PLATFORM}}", std::string(defaultPlatform));
  return content;
}

bool ProjectGenerator::ProcessTemplateFile(const std::filesystem::path& path,
                                           const std::string& projectName,
                                           const std::string& projectTarget,
                                           const std::filesystem::path& frameworkDirectory,
                                           std::string_view defaultPlatform) {
  std::ifstream input(path);

  if (!input)
    return false;

  std::string content{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};

  input.close();

  /*
   * Template values are replaced only in files explicitly processed by the
   * generator. Binary assets copied from the template are never modified.
   */
  content = RenderText(std::move(content), projectName, projectTarget, frameworkDirectory,
                       defaultPlatform);
  if (path.filename() == "levye.project") {
    for (auto position = content.find("\n\n\n"); position != std::string::npos;
         position = content.find("\n\n\n"))
      content.erase(position, 1);
  }

  std::ofstream output(path, std::ios::trunc);

  if (!output)
    return false;

  output << content;

  return output.good();
}

}  // namespace Levye
