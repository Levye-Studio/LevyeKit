#include "ProjectLoader.hpp"

#include <toml++/toml.hpp>

#include <iostream>

namespace Levye {

std::optional<ProjectConfig>
ProjectLoader::Load(const std::filesystem::path &path) {
  if (!std::filesystem::exists(path)) {
    std::cerr << "[LevyeKit] Project file does not exist: " << path << '\n';

    return std::nullopt;
  }

  toml::table table;

  try {
    table = toml::parse_file(path.string());
  } catch (const toml::parse_error &error) {
    std::cerr << "[LevyeKit] Failed to parse project file: "
              << error.description() << '\n';

    return std::nullopt;
  }

  ProjectConfig config;

  if (auto name = table["name"].value<std::string>()) {
    config.name = *name;
  }

  if (auto width = table["window"]["width"].value<int>()) {
    config.windowWidth = *width;
  }

  if (auto height = table["window"]["height"].value<int>()) {
    config.windowHeight = *height;
  }

  if (auto targetFPS = table["window"]["target_fps"].value<int>()) {
    config.targetFPS = *targetFPS;
  }

  if (auto resizable = table["window"]["resizable"].value<bool>()) {
    config.resizable = *resizable;
  }

  if (auto vsync = table["window"]["vsync"].value<bool>()) {
    config.vsync = *vsync;
  }

  if (auto assets = table["paths"]["assets"].value<std::string>()) {
    config.assetDirectory = *assets;
  }
  return config;
}

} // namespace Levye