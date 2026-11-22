#include "IOSExporter.hpp"

#include <Levye/Platform/ExecutablePath.hpp>
#include <Levye/Project/ProjectLoader.hpp>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <toml++/toml.hpp>

#include "../PlatformPreparation.hpp"

namespace Levye {
namespace {
namespace fs = std::filesystem;

bool Copy(const fs::path& source, const fs::path& destination) {
  std::error_code error;
  fs::create_directories(destination.parent_path(), error);
  if (error) return false;
  fs::copy(source, destination, fs::copy_options::recursive | fs::copy_options::overwrite_existing,
           error);
  return !error;
}

bool SafeAssets(const fs::path& project, const fs::path& relative) {
  if (relative.empty() || relative.is_absolute()) return false;
  for (const auto& component : relative)
    if (component == ".." || component == ".") return false;
  std::error_code error;
  const auto root = fs::canonical(project, error);
  if (error) return false;
  const auto assets = fs::canonical(project / relative, error);
  if (error || !fs::is_directory(assets)) return false;
  if (assets.string().rfind(root.string() + fs::path::preferred_separator, 0) != 0) return false;
  for (const auto& entry : fs::recursive_directory_iterator(assets, error)) {
    if (error || entry.is_symlink()) return false;
  }
  return !error;
}

bool NoSymlinks(const fs::path& directory) {
  std::error_code error;
  if (fs::is_symlink(directory) || !fs::is_directory(directory)) return false;
  for (const auto& entry : fs::recursive_directory_iterator(directory, error)) {
    if (error || entry.is_symlink()) return false;
  }
  return !error;
}

bool PortableSources(const fs::path& directory, const fs::path& project, const fs::path& framework,
                     const fs::path& cache) {
  for (const auto& entry : fs::recursive_directory_iterator(directory)) {
    if (!entry.is_regular_file()) continue;
    std::ifstream input(entry.path(), std::ios::binary);
    if (!input) return false;
    const std::string content{std::istreambuf_iterator<char>{input},
                              std::istreambuf_iterator<char>{}};
    for (const auto& original : {project, framework, cache})
      if (!original.empty() && content.find(original.string()) != std::string::npos) return false;
  }
  return true;
}

bool Write(const fs::path& path, const std::string& text) {
  std::ofstream output(path, std::ios::binary);
  output << text;
  return static_cast<bool>(output);
}

std::string CMakeLiteral(const std::string& value) {
  std::string escaped;
  for (char character : value) {
    if (character == '\\' || character == '"' || character == '$') escaped += '\\';
    escaped += character;
  }
  return "\"" + escaped + "\"";
}

}  // namespace

bool IOSExporter::Execute(const fs::path& projectDirectory, std::string_view format) {
  if (!format.empty()) {
    std::cerr << "[Levye] iOS export accepts no Android or Web format flags.\n";
    return false;
  }
#if !defined(__APPLE__)
  std::cerr << "[Levye] iOS export requires macOS and Xcode.\n";
  return false;
#else
  const auto project = ProjectLoader::Load(projectDirectory / "levye.project");
  if (!project) return false;
  if (!NoSymlinks(projectDirectory / "Source") ||
      !fs::is_regular_file(projectDirectory / "Source/Game.cpp") ||
      !SafeAssets(projectDirectory, project->assetDirectory)) {
    std::cerr
        << "[Levye] iOS export requires Source/Game.cpp and a project-contained asset directory.\n";
    return false;
  }
  std::string manifestText;
  bool changed = false;
  if (!PreparePlatformManifest(projectDirectory, ProjectPlatform::IOS, manifestText, changed))
    return false;
  const auto exportedProject = ProjectLoader::Parse(manifestText);
  if (!exportedProject) return false;
  std::string bundle = exportedProject->iosBundleId;
  if (bundle.empty()) {
    bundle = "com.levyekit.game";
    for (unsigned char character : exportedProject->target)
      if (std::isalnum(character)) bundle += static_cast<char>(std::tolower(character));
  }
  const auto framework = ExecutablePath::GetDirectory().parent_path().parent_path().parent_path();
  fs::path cache;
  if (const char* setting = std::getenv("LEVYE_DEPENDENCY_CACHE"); setting && *setting)
    cache = setting;
  else if (const char* setting = std::getenv("XDG_CACHE_HOME"); setting && *setting)
    cache = fs::path(setting) / "levye/dependencies";
  else if (const char* setting = std::getenv("HOME"); setting && *setting)
    cache = fs::path(setting) / ".cache/levye/dependencies";
  const auto raylib = cache / "raylib/ios-6.2/source";
  if (!PortableSources(projectDirectory / "Source", projectDirectory, framework, cache)) {
    std::cerr << "[Levye] Game source contains absolute paths to the original project, framework, "
                 "or cache.\n";
    return false;
  }
  fs::path toml;
  for (const auto& candidate : {framework / "Build/Release/_deps/tomlplusplus-src",
                                framework / "Build/Debug/_deps/tomlplusplus-src"}) {
    if (fs::is_regular_file(candidate / "CMakeLists.txt")) {
      toml = candidate;
      break;
    }
  }
  if (!fs::is_regular_file(raylib / "src/raylib.h") || !fs::is_directory(raylib / "deps/ANGLE") ||
      toml.empty()) {
    std::cerr
        << "[Levye] Cached iOS raylib/ANGLE or toml++ source is missing. Build an iOS app first.\n";
    return false;
  }
  const auto parent = projectDirectory / "Exports/iOS";
  const auto stage = parent / ".levye-stage";
  const auto destination = parent / project->target;
  std::error_code error;
  fs::create_directories(parent, error);
  if (error || !fs::create_directory(stage, error)) {
    std::cerr << "[Levye] Cannot create iOS export stage (another export may be active).\n";
    return false;
  }
  struct Cleanup {
    fs::path path;
    ~Cleanup() {
      std::error_code error;
      fs::remove_all(path, error);
    }
  } cleanup{stage};
  const auto game = stage / "Game";
  const auto kit = stage / "Framework";
  const auto deps = stage / "Dependencies";
  try {
    fs::create_directories(game);
    fs::create_directories(kit);
    fs::create_directories(deps / "raylib/deps");
    if (!Copy(projectDirectory / "Source", game / "Source") ||
        !Copy(projectDirectory / project->assetDirectory, game / project->assetDirectory) ||
        !Copy(framework / "Levye", kit / "Levye") || !Copy(framework / "cmake", kit / "cmake") ||
        !Copy(framework / "Platforms/iOS", kit / "Platforms/iOS") ||
        !Copy(framework / "CMakeLists.txt", kit / "CMakeLists.txt") ||
        !Copy(framework / "LICENSE", kit / "LICENSE") ||
        !Copy(raylib / "src", deps / "raylib/src") ||
        !Copy(raylib / "deps/ANGLE", deps / "raylib/deps/ANGLE") ||
        !Copy(framework / "Platforms/iOS/ANGLE-LICENSE", deps / "raylib/deps/ANGLE/LICENSE") ||
        !Copy(raylib / "LICENSE", deps / "raylib/LICENSE") ||
        !Copy(toml / "CMakeLists.txt", deps / "tomlplusplus/CMakeLists.txt") ||
        !Copy(toml / "cmake", deps / "tomlplusplus/cmake") ||
        !Copy(toml / "include", deps / "tomlplusplus/include") ||
        !Copy(toml / "LICENSE", deps / "tomlplusplus/LICENSE"))
      throw std::runtime_error("Cannot copy required source or dependencies.");
    auto manifest = toml::parse(manifestText);
    if (auto* android = manifest["android"].as_table()) android->erase("signing");
    std::ostringstream sanitized;
    sanitized << manifest;
    if (!Write(game / "levye.project", sanitized.str()))
      throw std::runtime_error("Cannot write exported project manifest.");
    const std::string wrapper =
        "cmake_minimum_required(VERSION 3.20)\n"
        "project(LevyeIOSExport LANGUAGES C CXX OBJC)\n"
        "set(LEVYE_ROOT \"${CMAKE_CURRENT_LIST_DIR}/Framework\")\n"
        "set(LEVYE_PROJECT_ROOT \"${CMAKE_CURRENT_LIST_DIR}/Game\")\n"
        "set(FETCHCONTENT_SOURCE_DIR_RAYLIB \"${CMAKE_CURRENT_LIST_DIR}/Dependencies/raylib\")\n"
        "set(FETCHCONTENT_SOURCE_DIR_TOMLPLUSPLUS "
        "\"${CMAKE_CURRENT_LIST_DIR}/Dependencies/tomlplusplus\")\n"
        "set(LEVYE_IOS_ASSET_DIRECTORY " +
        CMakeLiteral(exportedProject->assetDirectory) +
        ")\n"
        "set(LEVYE_IOS_BUNDLE_ID " +
        CMakeLiteral(bundle) +
        " CACHE STRING \"\" FORCE)\n"
        "set(LEVYE_IOS_DISPLAY_NAME " +
        CMakeLiteral(exportedProject->iosDisplayName.empty() ? exportedProject->name
                                                             : exportedProject->iosDisplayName) +
        " CACHE STRING \"\" FORCE)\n"
        "set(LEVYE_IOS_OUTPUT_NAME " +
        CMakeLiteral(exportedProject->target) +
        " CACHE STRING \"\" FORCE)\n"
        "set(LEVYE_IOS_VERSION_NAME " +
        CMakeLiteral(exportedProject->iosVersionName) +
        " CACHE STRING \"\" FORCE)\n"
        "set(LEVYE_IOS_VERSION_CODE " +
        std::to_string(exportedProject->iosVersionCode) +
        " CACHE STRING \"\" FORCE)\n"
        "set(LEVYE_IOS_DEPLOYMENT_TARGET " +
        CMakeLiteral(exportedProject->iosDeploymentTarget) +
        " CACHE STRING \"\" FORCE)\n"
        "set(LEVYE_IOS_ORIENTATION " +
        CMakeLiteral(exportedProject->displayOrientation == "auto"
                         ? "all"
                         : exportedProject->displayOrientation) +
        " CACHE STRING \"\" FORCE)\n"
        "set(CMAKE_OSX_DEPLOYMENT_TARGET " +
        CMakeLiteral(exportedProject->iosDeploymentTarget) +
        " CACHE STRING \"\" FORCE)\n"
        "add_subdirectory(\"${LEVYE_ROOT}/Platforms/iOS\" ios)\n";
    if (!Write(stage / "CMakeLists.txt", wrapper))
      throw std::runtime_error("Cannot write export CMake entry point.");
    const std::string instructions =
        "# LevyeKit iOS source export\n\n"
        "Move this entire directory freely. Generate the Xcode project after moving it; "
        "CMake-generated Xcode projects contain absolute paths.\n\n"
        "```sh\n"
        "cmake -S . -B Build/ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS "
        "-DCMAKE_OSX_SYSROOT=iphonesimulator -DCMAKE_OSX_ARCHITECTURES=arm64\n"
        "cmake --build Build/ios --config Debug\n"
        "cmake --build Build/ios --config Release\n"
        "```\n\n"
        "For a physical device, configure a separate project:\n\n"
        "```sh\n"
        "cmake -S . -B Build/device -G Xcode -DCMAKE_SYSTEM_NAME=iOS "
        "-DCMAKE_OSX_SYSROOT=iphoneos -DCMAKE_OSX_ARCHITECTURES=arm64\n"
        "open Build/device/LevyeIOSExport.xcodeproj\n"
        "```\n\n"
        "Set your Apple development team and provisioning profile in Xcode, "
        "then build for your connected device. Device signing and runtime have "
        "not been verified.\n\n"
        "Licenses: Framework/LICENSE, Dependencies/raylib/LICENSE, "
        "Dependencies/tomlplusplus/LICENSE, Dependencies/raylib/deps/ANGLE/LICENSE.\n";
    if (!Write(stage / "README.md", instructions))
      throw std::runtime_error("Cannot write export instructions.");
    const auto previous = parent / ".levye-previous";
    if (fs::exists(previous) || fs::is_symlink(destination))
      throw std::runtime_error("Cannot replace existing iOS export safely.");
    const bool replacing = fs::exists(destination);
    if (replacing) fs::rename(destination, previous);
    try {
      fs::rename(stage, destination);
    } catch (...) {
      if (replacing) fs::rename(previous, destination);
      throw;
    }
    cleanup.path.clear();
    if (replacing) fs::remove_all(previous);
    std::cout << "[Levye] iOS source exported: " << destination << '\n';
    return true;
  } catch (const std::exception& exception) {
    std::cerr << "[Levye] iOS export failed: " << exception.what() << '\n';
    return false;
  }
#endif
}

}  // namespace Levye
