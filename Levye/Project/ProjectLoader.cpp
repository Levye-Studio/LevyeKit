#include "ProjectLoader.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <string>
#include <string_view>
#include <toml++/toml.hpp>

namespace Levye {
namespace {

int EditDistance(std::string_view left, std::string_view right) {
  if (left.size() > 64 || right.size() > 64)
    return 65;
  int previous[65]{};
  int current[65]{};
  for (std::size_t j = 0; j <= right.size(); ++j)
    previous[j] = static_cast<int>(j);
  for (std::size_t i = 1; i <= left.size(); ++i) {
    current[0] = static_cast<int>(i);
    for (std::size_t j = 1; j <= right.size(); ++j)
      current[j] = std::min({previous[j] + 1, current[j - 1] + 1,
                             previous[j - 1] + (left[i - 1] == right[j - 1] ? 0 : 1)});
    std::copy_n(current, right.size() + 1, previous);
  }
  return previous[right.size()];
}

void WarnUnknown(const toml::table& table, std::string_view section,
                 std::initializer_list<std::string_view> known) {
  for (const auto& [key, ignored] : table) {
    const std::string_view name = key.str();
    if (std::find(known.begin(), known.end(), name) != known.end())
      continue;
    std::string_view suggestion;
    int best = 3;
    for (const auto candidate : known) {
      const int distance = EditDistance(name, candidate);
      if (distance < best) {
        best = distance;
        suggestion = candidate;
      }
    }
    std::cerr << "[LevyeKit] Warning: unknown [" << section << "] key '" << name << "'";
    if (!suggestion.empty())
      std::cerr << "; did you mean '" << suggestion << "'?";
    std::cerr << " Custom keys are preserved.\n";
  }
}

bool ValidateSchema(const toml::table& root) {
  struct Section {
    const char* name;
    std::initializer_list<std::string_view> keys;
  };
  const Section sections[] = {
      {"project", {"name", "target", "template_version"}},
      {"platform", {"default"}},
      {"display",
       {"orientation", "logical_width", "logical_height", "scale_mode", "filter", "fullscreen",
        "safe_area", "target_fps", "vsync", "viewport_width", "viewport_height", "viewport_mode",
        "scaling", "window_width", "window_height"}},
      {"desktop",
       {"window_width", "window_height", "resizable", "app_name", "bundle_id", "version_name",
        "version_code", "icon"}},
      {"window", {"width", "height", "resizable", "target_fps", "vsync"}},
      {"paths", {"assets"}},
      {"android",
       {"package", "orientation", "app_name", "version_name", "version_code", "icon",
        "adaptive_icon_foreground", "adaptive_icon_background", "signing"}},
      {"ios",
       {"bundle_id", "deployment_target", "display_name", "version_name", "version_code",
        "orientation"}},
      {"modules", {"serialization", "imgui", "animation", "physics"}},
  };
  for (const auto& section : sections) {
    const auto node = root[section.name];
    if (!node.node())
      continue;
    const auto* values = node.as_table();
    if (!values) {
      std::cerr << "[LevyeKit] [" << section.name << "] must be a table.\n";
      return false;
    }
    WarnUnknown(*values, section.name, section.keys);
  }
  for (const auto& [key, ignored] : root) {
    const std::string_view name = key.str();
    if (std::find_if(std::begin(sections), std::end(sections), [name](const Section& section) {
          return name == section.name;
        }) == std::end(sections))
      std::cerr << "[LevyeKit] Warning: unknown section [" << name
                << "]; custom sections are preserved.\n";
  }
  for (const auto key : {"name", "target"}) {
    const auto node = root["project"][key];
    if (node.node() && !node.is_string()) {
      std::cerr << "[LevyeKit] [project] " << key << " must be a string.\n";
      return false;
    }
  }
  if (const auto node = root["project"]["template_version"];
      node.node() && (!node.is_integer() || node.value<std::int64_t>().value_or(-1) < 0)) {
    std::cerr << "[LevyeKit] [project] template_version must be a nonnegative integer.\n";
    return false;
  }
  if (const auto node = root["paths"]["assets"];
      node.node() && (!node.is_string() || node.value<std::string>()->empty())) {
    std::cerr << "[LevyeKit] [paths] assets must be a nonempty string.\n";
    return false;
  }
  if (const auto node = root["android"]["package"];
      node.node() && (!node.is_string() || node.value<std::string>()->empty())) {
    std::cerr << "[LevyeKit] [android] package must be a nonempty string.\n";
    return false;
  }
  if (const auto* modules = root["modules"].as_table()) {
    for (const auto key : {"serialization", "imgui", "animation", "physics"})
      if (const auto node = (*modules)[key]; node.node() && !node.is_boolean()) {
        std::cerr << "[LevyeKit] [modules] " << key << " must be a boolean.\n";
        return false;
      }
  }
  return true;
}

}  // namespace

std::optional<ProjectConfig> ProjectLoader::Load(const std::filesystem::path& path) {
  if (!std::filesystem::exists(path)) {
    std::cerr << "[LevyeKit] Project file does not exist: " << path << '\n';

    return std::nullopt;
  }

  std::ifstream file(path, std::ios::binary);

  if (!file) {
    std::cerr << "[LevyeKit] Cannot open project file: " << path << '\n';
    return std::nullopt;
  }

  const std::string content{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};

  if (file.bad()) {
    std::cerr << "[LevyeKit] Failed reading project file: " << path << '\n';
    return std::nullopt;
  }

  return Parse(content);
}

std::optional<ProjectConfig> ProjectLoader::Parse(std::string_view content) {
  toml::table table;

  try {
    table = toml::parse(content);
  } catch (const toml::parse_error& error) {
    std::cerr << "[LevyeKit] Failed to parse project file: " << error.description() << '\n';

    return std::nullopt;
  }

  if (!ValidateSchema(table))
    return std::nullopt;

  ProjectConfig config;

  if (auto name = table["project"]["name"].value<std::string>()) {
    config.name = *name;
  }

  if (auto target = table["project"]["target"].value<std::string>()) {
    config.target = *target;
  }

  // Legacy [window] settings remain supported for projects that have not been upgraded.
  // Validate explicitly provided values rather than silently falling back to defaults.
  if (const auto node = table["window"]; node.node() && !node.is_table()) {
    std::cerr << "[LevyeKit] [window] must be a table.\n";
    return std::nullopt;
  }
  for (const auto [key, output] :
       {std::pair{"width", &config.windowWidth}, std::pair{"height", &config.windowHeight},
        std::pair{"target_fps", &config.targetFPS}}) {
    if (const auto node = table["window"][key]; node.node()) {
      const auto value = node.value<int>();
      if (!value || *value <= 0) {
        std::cerr << "[LevyeKit] [window] " << key << " must be a positive integer.\n";
        return std::nullopt;
      }
      *output = *value;
    }
  }
  for (const auto [key, output] :
       {std::pair{"resizable", &config.resizable}, std::pair{"vsync", &config.vsync}}) {
    if (const auto node = table["window"][key]; node.node()) {
      const auto value = node.value<bool>();
      if (!value) {
        std::cerr << "[LevyeKit] [window] " << key << " must be a boolean.\n";
        return std::nullopt;
      }
      *output = *value;
    }
  }

  if (table["display"].node() && !table["display"].as_table()) {
    std::cerr << "[LevyeKit] [display] must be a table.\n";
    return std::nullopt;
  }
  // Modern frame pacing settings live in [display]. Legacy [window] values
  // remain accepted, but contradictory explicit settings are not silently ignored.
  for (const auto [key, output] : {std::pair{"target_fps", &config.targetFPS}}) {
    if (const auto node = table["display"][key]; node.node()) {
      const auto value = node.value<int>();
      if (!value || *value <= 0) {
        std::cerr << "[LevyeKit] [display] " << key << " must be a positive integer.\n";
        return std::nullopt;
      }
      if (const auto legacy = table["window"][key]; legacy.node() && legacy.value<int>() != value) {
        std::cerr << "[LevyeKit] Conflicting [window] and [display] " << key
                  << "; run levye upgrade.\n";
        return std::nullopt;
      }
      *output = *value;
    }
  }
  if (const auto node = table["display"]["vsync"]; node.node()) {
    const auto value = node.value<bool>();
    if (!value) {
      std::cerr << "[LevyeKit] [display] vsync must be a boolean.\n";
      return std::nullopt;
    }
    if (const auto legacy = table["window"]["vsync"];
        legacy.node() && legacy.value<bool>() != value) {
      std::cerr << "[LevyeKit] Conflicting [window] and [display] vsync; run levye upgrade.\n";
      return std::nullopt;
    }
    config.vsync = *value;
  }
  if (const auto* display = table["display"].as_table()) {
    for (const auto [key, output] : {std::pair{"window_width", &config.windowWidth},
                                     std::pair{"window_height", &config.windowHeight}}) {
      if (const auto entry = (*display)[key]; entry.node()) {
        const auto value = entry.value<int>();
        if (!value || *value <= 0) {
          std::cerr << "[LevyeKit] Legacy [display] " << key << " must be positive.\n";
          return std::nullopt;
        }
        const std::string_view oldKey =
            std::string_view(key) == "window_width" ? "width" : "height";
        if (const auto legacy = table["window"][oldKey].value<int>(); legacy && *legacy != *value) {
          std::cerr << "[LevyeKit] Conflicting [window] " << oldKey << " and [display] " << key
                    << "; run levye upgrade.\n";
          return std::nullopt;
        }
        *output = *value;
      }
    }
    const auto width = (*display)["logical_width"].node()
                           ? (*display)["logical_width"].value<int>()
                           : (*display)["viewport_width"].value<int>();
    const auto height = (*display)["logical_height"].node()
                            ? (*display)["logical_height"].value<int>()
                            : (*display)["viewport_height"].value<int>();
    const auto scalingNode = (*display)["scale_mode"].node() ? (*display)["scale_mode"]
                             : (*display)["scaling"].node()  ? (*display)["scaling"]
                                                             : (*display)["viewport_mode"];
    const auto scaling = scalingNode.value<std::string>();
    const bool conflictingWidth =
        (*display)["logical_width"].node() && (*display)["viewport_width"].node() &&
        (*display)["logical_width"].value<int>() != (*display)["viewport_width"].value<int>();
    const bool conflictingHeight =
        (*display)["logical_height"].node() && (*display)["viewport_height"].node() &&
        (*display)["logical_height"].value<int>() != (*display)["viewport_height"].value<int>();
    const auto modernMode = (*display)["scale_mode"].value<std::string>();
    const auto oldMode = (*display)["scaling"].value<std::string>();
    const auto oldestMode = (*display)["viewport_mode"].value<std::string>();
    const bool conflictingMode = (modernMode && oldMode && modernMode != oldMode) ||
                                 (modernMode && oldestMode && modernMode != oldestMode) ||
                                 (oldMode && oldestMode && oldMode != oldestMode);
    if (!width || !height || *width <= 0 || *height <= 0 || (scalingNode.node() && !scaling) ||
        conflictingWidth || conflictingHeight || conflictingMode) {
      std::cerr << "[LevyeKit] [display] requires positive logical_width and logical_height, "
                   "and one consistent string scale_mode.\n";
      return std::nullopt;
    }
    config.useLogicalDisplay = true;
    config.display.logical = {*width, *height};
    if (!config.hasDisplayOrientation)
      config.displayOrientation = *height > *width ? "portrait" : "landscape";
    if (scaling) {
      if (*scaling == "fit")
        config.display.scaling = DisplayScaling::Fit;
      else if (*scaling == "fill")
        config.display.scaling = DisplayScaling::Fill;
      else if (*scaling == "stretch")
        config.display.scaling = DisplayScaling::Stretch;
      else if (*scaling == "integer")
        config.display.scaling = DisplayScaling::Integer;
      else if (*scaling == "expand")
        config.display.scaling = DisplayScaling::Expand;
      else {
        std::cerr << "[LevyeKit] Unsupported display scaling: " << *scaling << '\n';
        return std::nullopt;
      }
    }
    if (const auto node = (*display)["filter"]; node.node()) {
      const auto value = node.value<std::string>();
      if (!value || (*value != "nearest" && *value != "linear")) {
        std::cerr << "[LevyeKit] [display] filter must be nearest or linear.\n";
        return std::nullopt;
      }
      config.display.filter = *value == "nearest" ? DisplayFilter::Nearest : DisplayFilter::Linear;
    }
    if (const auto node = (*display)["safe_area"]; node.node()) {
      const auto value = node.value<std::string>();
      if (!value || (*value != "inset" && *value != "edge_to_edge")) {
        std::cerr << "[LevyeKit] [display] safe_area must be inset or edge_to_edge.\n";
        return std::nullopt;
      }
      config.display.safeArea =
          *value == "inset" ? DisplaySafeArea::Inset : DisplaySafeArea::EdgeToEdge;
    }
    if (const auto node = (*display)["orientation"]; node.node()) {
      const auto value = node.value<std::string>();
      if (!value || (*value != "portrait" && *value != "landscape" && *value != "auto")) {
        std::cerr << "[LevyeKit] [display] orientation must be portrait, landscape, or auto.\n";
        return std::nullopt;
      }
      config.displayOrientation = *value;
      config.hasDisplayOrientation = true;
    }
    if (const auto node = (*display)["fullscreen"]; node.node() && !node.is_boolean()) {
      std::cerr << "[LevyeKit] [display] fullscreen must be a boolean.\n";
      return std::nullopt;
    }
    config.fullscreen = (*display)["fullscreen"].value_or(false);
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
    if (*platform != "desktop" && *platform != "android" && *platform != "ios" &&
        *platform != "web") {
      std::cerr << "[LevyeKit] Unsupported default platform: " << *platform << '\n';
      return std::nullopt;
    }
    config.defaultPlatform = *platform;
  }
  if (const auto node = table["ios"]; node.node()) {
    const auto* ios = node.as_table();
    if (!ios) {
      std::cerr << "[LevyeKit] [ios] must be a table.\n";
      return std::nullopt;
    }
    auto readString = [&](std::string_view key, std::string& output) {
      const auto entry = (*ios)[key];
      if (!entry.node())
        return true;
      const auto value = entry.value<std::string>();
      if (!value || value->empty()) {
        std::cerr << "[LevyeKit] [ios] " << key << " must be a nonempty string.\n";
        return false;
      }
      output = *value;
      return true;
    };
    if (!readString("bundle_id", config.iosBundleId) ||
        !readString("deployment_target", config.iosDeploymentTarget) ||
        !readString("display_name", config.iosDisplayName) ||
        !readString("version_name", config.iosVersionName) ||
        !readString("orientation", config.iosOrientation))
      return std::nullopt;
    static const std::regex bundlePattern(R"([A-Za-z][A-Za-z0-9-]*(\.[A-Za-z][A-Za-z0-9-]*)+)");
    static const std::regex deploymentPattern(R"(([0-9]{1,3})\.([0-9]{1,3}))");
    static const std::regex versionPattern(R"([0-9]+(\.[0-9]+){0,2})");
    std::smatch deployment;
    if ((!config.iosBundleId.empty() && !std::regex_match(config.iosBundleId, bundlePattern)) ||
        !std::regex_match(config.iosDeploymentTarget, deployment, deploymentPattern) ||
        std::stoi(deployment[1].str()) < 15 ||
        !std::regex_match(config.iosVersionName, versionPattern) ||
        (config.iosOrientation != "all" && config.iosOrientation != "portrait" &&
         config.iosOrientation != "landscape")) {
      std::cerr << "[LevyeKit] Invalid iOS bundle ID, deployment target, or version.\n";
      return std::nullopt;
    }
    if (const auto entry = (*ios)["version_code"]; entry.node()) {
      const auto value = entry.value_exact<std::int64_t>();
      if (!value || *value <= 0 || *value > 2147483647) {
        std::cerr << "[LevyeKit] [ios] version_code must be a positive integer.\n";
        return std::nullopt;
      }
      config.iosVersionCode = static_cast<int>(*value);
    }
  }
  if (const auto node = table["desktop"]; node.node()) {
    const auto* desktop = node.as_table();
    if (!desktop) {
      std::cerr << "[LevyeKit] [desktop] must be a table.\n";
      return std::nullopt;
    }
    auto readString = [&](std::string_view key, std::string& output) {
      const auto entry = (*desktop)[key];
      if (!entry.node())
        return true;
      const auto value = entry.value<std::string>();
      if (!value || value->empty() || value->find('\0') != std::string::npos) {
        std::cerr << "[LevyeKit] [desktop] " << key << " must be a nonempty string.\n";
        return false;
      }
      output = *value;
      return true;
    };
    if (!readString("app_name", config.desktopAppName) ||
        !readString("bundle_id", config.desktopBundleId) ||
        !readString("version_name", config.desktopVersionName) ||
        !readString("icon", config.desktopIcon))
      return std::nullopt;
    for (const auto [key, output] : {std::pair{"window_width", &config.windowWidth},
                                     std::pair{"window_height", &config.windowHeight}}) {
      if (const auto entry = (*desktop)[key]; entry.node()) {
        const auto value = entry.value<int>();
        if (!value || *value <= 0) {
          std::cerr << "[LevyeKit] [desktop] " << key << " must be positive.\n";
          return std::nullopt;
        }
        const std::string_view oldWindowKey =
            std::string_view(key) == "window_width" ? "width" : "height";
        if (const auto legacy = table["window"][oldWindowKey].value<int>();
            legacy && *legacy != *value) {
          std::cerr << "[LevyeKit] Conflicting [window] " << oldWindowKey << " and [desktop] "
                    << key << "; run levye upgrade.\n";
          return std::nullopt;
        }
        if (const auto legacy = table["display"][key].value<int>(); legacy && *legacy != *value) {
          std::cerr << "[LevyeKit] Conflicting [display] and [desktop] " << key
                    << "; run levye upgrade.\n";
          return std::nullopt;
        }
        *output = *value;
      }
    }
    if (const auto entry = (*desktop)["resizable"]; entry.node()) {
      const auto value = entry.value<bool>();
      if (!value) {
        std::cerr << "[LevyeKit] [desktop] resizable must be a boolean.\n";
        return std::nullopt;
      }
      if (const auto legacy = table["window"]["resizable"].value<bool>();
          legacy && *legacy != *value) {
        std::cerr << "[LevyeKit] Conflicting [window] and [desktop] resizable; "
                     "run levye upgrade.\n";
        return std::nullopt;
      }
      config.resizable = *value;
    }
    if (const auto entry = (*desktop)["version_code"]; entry.node()) {
      const auto value = entry.value_exact<std::int64_t>();
      if (!value || *value <= 0 || *value > 2147483647) {
        std::cerr << "[LevyeKit] [desktop] version_code must be a positive integer.\n";
        return std::nullopt;
      }
      config.desktopVersionCode = static_cast<int>(*value);
    }
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
    if (config.hasDisplayOrientation && config.displayOrientation != *orientation) {
      std::cerr
          << "[LevyeKit] Conflicting [display] and [android] orientations; run levye upgrade.\n";
      return std::nullopt;
    }
    if (!config.hasDisplayOrientation)
      config.displayOrientation = *orientation;
  }
  if (auto orientation = table["ios"]["orientation"].value<std::string>()) {
    const std::string legacy = *orientation == "all" ? "auto" : *orientation;
    if (config.hasDisplayOrientation && config.displayOrientation != legacy) {
      std::cerr << "[LevyeKit] Conflicting [display] and [ios] orientations; run levye upgrade.\n";
      return std::nullopt;
    }
    if (!config.hasDisplayOrientation && table["android"]["orientation"].node() &&
        config.displayOrientation != legacy) {
      std::cerr
          << "[LevyeKit] Conflicting legacy [android] and [ios] orientations; run levye upgrade.\n";
      return std::nullopt;
    }
    if (!config.hasDisplayOrientation)
      config.displayOrientation = legacy;
  }
  if (!config.hasDisplayOrientation && !table["android"]["orientation"].node() &&
      !table["ios"]["orientation"].node()) {
    if (table["ios"].node())
      config.displayOrientation = "auto";
    else if (table["android"].node())
      config.displayOrientation = "portrait";
  }
  const auto* android = table["android"].as_table();
  if (android) {
    auto readString = [&](std::string_view key, std::string& output) -> bool {
      const auto node = (*android)[key];
      if (!node.node())
        return true;
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
        if (!node.node())
          return true;
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
