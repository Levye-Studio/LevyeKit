#include "ProjectLoader.hpp"

#include <Levye/Debug/Logger.hpp>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <toml++/toml.hpp>

namespace Levye {

std::optional<ProjectConfig> ProjectLoader::Load(const std::filesystem::path& path) {
  if (!std::filesystem::exists(path)) {
    std::cerr << "[LevyeKit] Project file does not exist: " << path << '\n';

    return std::nullopt;
  }

  std::ifstream file(path, std::ios::binary);

  if (!file) {
    // std::string errorMsg = "[LevyeKit] Cannot open project file: " + path.string();
    // Logger::Error(errorMsg.c_str());
    std::cerr << "[LevyeKit] Cannot open project file: " << path << '\n';
    return std::nullopt;
  }

  const std::string content{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};

  if (file.bad()) {
    // std::string errorMsg = "[LevyeKit] Failed reading project file: " + path.string();
    // Logger::Error(errorMsg.c_str());
    std::cerr << "[LevyeKit] Failed reading project file: " << path << '\n';
    return std::nullopt;
  }

  return Parse(content);
}

std::optional<ProjectConfig> ProjectLoader::Parse(std::string_view content) {
  toml::table table;

  try {
    // table = toml::parse_file(path.string());
    table = toml::parse(content);
  } catch (const toml::parse_error& error) {
    // std::string errorMsg =
    //     "[LevyeKit] Failed to parse project file: " + std::string(error.description());
    std::cerr << "[LevyeKit] Failed to parse project file: " << error.description() << '\n';

    return std::nullopt;
  }

  ProjectConfig config;

  if (auto name = table["project"]["name"].value<std::string>()) {
    config.name = *name;
  }

  if (auto target = table["project"]["target"].value<std::string>()) {
    config.target = *target;
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

  if (const auto* display = table["display"].as_table()) {
    const auto width = (*display)["logical_width"].value<int>();
    const auto height = (*display)["logical_height"].value<int>();
    const auto scaling = (*display)["scaling"].value<std::string>();
    if (!width || !height || *width <= 0 || *height <= 0 ||
        ((*display)["scaling"].node() && !scaling)) {
      std::cerr << "[LevyeKit] [display] requires positive logical_width and logical_height, "
                   "and a string scaling mode.\n";
      return std::nullopt;
    }
    config.useLogicalDisplay = true;
    config.display.logical = {*width, *height};
    if (scaling) {
      if (*scaling == "fit")
        config.display.scaling = DisplayScaling::Fit;
      else if (*scaling == "fill")
        config.display.scaling = DisplayScaling::Fill;
      else if (*scaling == "stretch")
        config.display.scaling = DisplayScaling::Stretch;
      else {
        std::cerr << "[LevyeKit] Unsupported display scaling: " << *scaling << '\n';
        return std::nullopt;
      }
    }
  }

  if (auto assets = table["paths"]["assets"].value<std::string>()) {
    config.assetDirectory = *assets;
  }

  if (const auto* platformNode = table["platform"]["default"].node();
      platformNode && !platformNode->is_string()) {
    std::cerr << "[LevyeKit] Default platform must be a string.\n";
    return std::nullopt;
  }
  if (auto platform = table["platform"]["default"].value<std::string>()) {
    if (*platform != "desktop" && *platform != "android") {
      std::cerr << "[LevyeKit] Unsupported default platform: " << *platform << '\n';
      return std::nullopt;
    }
    config.defaultPlatform = *platform;
  }
  if (auto package = table["android"]["package"].value<std::string>()) {
    config.androidPackage = *package;
  }
  if (auto orientation = table["android"]["orientation"].value<std::string>()) {
    if (*orientation != "portrait" && *orientation != "landscape") {
      std::cerr << "[LevyeKit] Unsupported Android orientation: " << *orientation << '\n';
      return std::nullopt;
    }
    config.androidOrientation = *orientation;
  }
  const auto* android = table["android"].as_table();
  if (android) {
    auto readString = [&](std::string_view key, std::string& output) -> bool {
      const auto node = (*android)[key];
      if (!node.node()) return true;
      const auto value = node.value<std::string>();
      if (!value || value->empty()) {
        std::cerr << "[LevyeKit] [android] " << key << " must be a nonempty string.\n";
        return false;
      }
      output = *value;
      return true;
    };
    if (!readString("app_name", config.androidAppName) ||
        !readString("version_name", config.androidVersionName) ||
        !readString("icon", config.androidIcon) ||
        !readString("adaptive_icon_foreground", config.androidAdaptiveIconForeground) ||
        !readString("adaptive_icon_background", config.androidAdaptiveIconBackground))
      return std::nullopt;
    if (const auto node = (*android)["version_code"]; node.node()) {
      const auto value = node.value<int>();
      if (!value || *value <= 0) {
        std::cerr << "[LevyeKit] [android] version_code must be a positive integer.\n";
        return std::nullopt;
      }
      config.androidVersionCode = *value;
    }
    if (const auto signingNode = (*android)["signing"]; signingNode.node()) {
      const auto* signing = signingNode.as_table();
      if (!signing) {
        std::cerr << "[LevyeKit] [android.signing] must be a table.\n";
        return std::nullopt;
      }
      for (const auto& [key, value] : *signing) {
        if (key.str() != "keystore" && key.str() != "alias") {
          std::cerr << "[LevyeKit] Unsupported [android.signing] option: " << key << '\n';
          return std::nullopt;
        }
      }
      auto readSigningString = [&](std::string_view key, std::string& output) -> bool {
        const auto node = (*signing)[key];
        if (!node.node()) return true;
        const auto value = node.value<std::string>();
        if (!value || value->empty()) {
          std::cerr << "[LevyeKit] [android.signing] " << key << " must be a nonempty string.\n";
          return false;
        }
        output = *value;
        return true;
      };
      if (!readSigningString("keystore", config.androidSigningKeystore) ||
          !readSigningString("alias", config.androidSigningAlias))
        return std::nullopt;
    }
  }

  if (config.name.empty()) {
    std::cerr << "[LevyeKit] Project name cannot be empty.\n";

    return std::nullopt;
  }

  if (config.target.empty()) {
    std::cerr << "[LevyeKit] Project target cannot be empty.\n";

    return std::nullopt;
  }

  return config;
}

}  // namespace Levye
