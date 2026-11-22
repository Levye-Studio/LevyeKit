#include "PlatformPreparation.hpp"

#include <Levye/Project/ProjectLoader.hpp>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <toml++/toml.hpp>

namespace Levye {

bool PreparePlatformManifest(const std::filesystem::path& projectDirectory,
                             ProjectPlatform platform, std::string& content, bool& changed) {
  changed = false;
  std::ifstream input(projectDirectory / "levye.project", std::ios::binary);
  if (!input) {
    std::cerr << "[Levye] Cannot read levye.project.\n";
    return false;
  }

  content.assign(std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{});

  if (input.bad()) return false;

  auto project = ProjectLoader::Parse(content);

  if (!project) return false;
  toml::table manifest;
  try {
    manifest = toml::parse(content);
  } catch (const toml::parse_error& error) {
    std::cerr << "[Levye] Invalid levye.project: " << error << '\n';
    return false;
  }
  for (const char* module : {"serialization", "imgui", "animation", "physics"}) {
    if (platform != ProjectPlatform::Desktop && manifest["modules"][module].value_or(false)) {
      std::cerr << "[Levye] " << (platform == ProjectPlatform::IOS ? "iOS" : "Android")
                << " does not support optional module '" << module << "'.\n";
      return false;
    }
  }
  auto append = [&](const std::string& section) {
    if (!content.empty() && content.back() != '\n') content += '\n';
    content += '\n' + section;
    changed = true;
  };
  if (platform != ProjectPlatform::Desktop && !manifest["display"].node()) {
    const std::string orientation = manifest["android"].node() || manifest["ios"].node()
                                        ? project->displayOrientation
                                    : platform == ProjectPlatform::Android ? "portrait"
                                                                           : "auto";
    const bool portrait = orientation == "portrait";
    append(
        "[display]\norientation = \"" + orientation +
        "\"\nlogical_width = " + std::string(portrait ? "720" : "1280") +
        "\nlogical_height = " + std::string(portrait ? "1280" : "720") +
        "\nscale_mode = \"fit\"\nfilter = \"linear\"\nfullscreen = true\nsafe_area = \"inset\"\n");
  }
  if (platform == ProjectPlatform::Desktop) {
    if (!manifest["desktop"].node())
      append("[desktop]\nwindow_width = " + std::to_string(project->windowWidth) +
             "\nwindow_height = " + std::to_string(project->windowHeight) +
             "\nresizable = " + (project->resizable ? "true" : "false") + "\n");
  } else if (platform == ProjectPlatform::IOS) {
    const auto node = manifest["ios"];
    if (node.node() && !node.as_table()) {
      std::cerr << "[Levye] [ios] must be a table.\n";
      return false;
    }
    if (!node.node()) {
      std::string bundle = "com.levyekit.game";
      for (unsigned char character : project->target)
        if (std::isalnum(character)) bundle += static_cast<char>(std::tolower(character));
      append("[ios]\nbundle_id = \"" + bundle + "\"\ndeployment_target = \"15.0\"\n");
    }
  } else if (platform == ProjectPlatform::Android) {
    if (!manifest["android"].node()) {
      std::string bundle = "com.levyekit.game";
      for (unsigned char character : project->target)
        if (std::isalnum(character)) bundle += static_cast<char>(std::tolower(character));
      append("[android]\npackage = \"" + bundle + "\"\n");
    } else if (project->androidPackage.empty()) {
      std::cerr << "[Levye] Set [android] package in levye.project.\n";
      return false;
    }
  }
  const auto prepared = ProjectLoader::Parse(content);
  if (!prepared) return false;
  if (platform == ProjectPlatform::Android) {
    if (prepared->displayOrientation == "auto") {
      std::cerr << "[Levye] Android auto orientation is unavailable: raylib 6.2 retains stale "
                   "drawable dimensions after rotation. Choose portrait or landscape in "
                   "[display].orientation.\n";
      return false;
    }
    static const std::regex packagePattern(R"([A-Za-z][A-Za-z0-9_]*(\.[A-Za-z][A-Za-z0-9_]*)+)");
    if (!std::regex_match(prepared->androidPackage, packagePattern)) {
      std::cerr << "[Levye] Invalid [android] package identifier.\n";
      return false;
    }
  }
  return true;
}

}  // namespace Levye
