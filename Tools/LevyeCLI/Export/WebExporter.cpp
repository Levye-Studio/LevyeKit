#include "WebExporter.hpp"

#include <Levye/Platform/ExecutablePath.hpp>
#include <Levye/Project/ProjectLoader.hpp>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <toml++/toml.hpp>
#include <vector>

#include "../ProcessArguments.hpp"

namespace Levye {
namespace fs = std::filesystem;
bool WebExporter::Execute(const fs::path& directory, std::string_view format) {
  if (!format.empty() && format != "zip") {
    std::cerr << "[Levye] Android formats cannot be used for HTML export.\n";
    return false;
  }
#ifdef _WIN32
  std::cerr << "[Levye] HTML export currently requires a macOS or Linux host.\n";
  return false;
#else
  try {
    const auto root = fs::canonical(directory);
    const auto framework = ExecutablePath::GetDirectory().parent_path().parent_path().parent_path();
    const auto project = ProjectLoader::Load(root / "levye.project");
    if (!project) return false;
    auto manifest = toml::parse_file((root / "levye.project").string());
    for (const auto* module : {"serialization", "imgui", "animation", "physics"})
      if (manifest["modules"][module].value_or(false))
        throw std::runtime_error(std::string("HTML does not yet support ") + module);
    const fs::path assets(project->assetDirectory);
    if (assets.empty() || assets.is_absolute() || assets == "." ||
        *assets.begin() == "levye.project" || *assets.begin() == "Exports" ||
        *assets.begin() == "Build" || *assets.begin() == "Targets")
      throw std::runtime_error("Invalid asset directory.");
    for (const auto& part : assets)
      if (part == ".." || part == ".") throw std::runtime_error("Invalid asset path.");
    auto source = root;
    for (const auto& part : assets) {
      source /= part;
      if (fs::is_symlink(source)) throw std::runtime_error("Asset symlinks are unsupported.");
    }
    const auto output = root / "Exports" / "HTML";
    if (fs::is_symlink(root / "Exports") || fs::is_symlink(output))
      throw std::runtime_error("Export directory symlinks are unsupported.");
    fs::create_directories(output);
    const auto stage = output / ".levye-stage";
    if (!fs::create_directory(stage)) throw std::runtime_error("HTML export stage already exists.");
    struct Cleanup {
      fs::path path;
      ~Cleanup() {
        std::error_code ec;
        fs::remove_all(path, ec);
      }
    } cleanup{stage};
    const auto payload = stage / "payload";
    const auto package = stage / "package";
    fs::create_directories(payload / assets);
    fs::create_directories(package);
    for (const auto& entry : fs::recursive_directory_iterator(source)) {
      if (entry.is_symlink()) throw std::runtime_error("Asset symlinks are unsupported.");
      if (entry.is_directory()) continue;
      if (!entry.is_regular_file()) throw std::runtime_error("Invalid asset file.");
      auto ext = entry.path().extension().string();
      std::transform(ext.begin(), ext.end(), ext.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      if (ext == ".jks" || ext == ".p12" || ext == ".pfx" || ext == ".keystore")
        throw std::runtime_error("Signing keys cannot be exported.");
      const auto dest = payload / assets / entry.path().lexically_relative(source);
      fs::create_directories(dest.parent_path());
      fs::copy_file(entry.path(), dest);
    }
    if (auto* android = manifest["android"].as_table()) android->erase("signing");
    {
      std::ofstream stream(payload / "levye.project");
      stream << manifest;
      if (!stream) return false;
    }
    const auto build = root / "Build" / "WebRelease-4.0.20";
    auto q = [](const fs::path& p) { return LevyeCLI::QuoteArgument(p.string()); };
    const auto sdk = "bash " + q(framework / "Platforms/Web/with-emsdk.sh") + " ";
    std::string command = sdk + "emcmake cmake -S " + q(framework / "Platforms/Web") + " -B " + q(build) +
                          " -DCMAKE_BUILD_TYPE=Release " + q("-DLEVYE_ROOT=" + framework.string()) +
                          " " + q("-DLEVYE_PROJECT_ROOT=" + root.string()) + " " +
                          q("-DLEVYE_WEB_OUTPUT=" + package.string()) + " " +
                          q("-DLEVYE_WEB_PAYLOAD=" + payload.string());
    if (std::system(command.c_str()) != 0 ||
        std::system((sdk + "cmake --build " + q(build) + " --config Release").c_str()) != 0)
      return false;
    for (const auto* name : {"index.html", "index.js", "index.wasm", "index.data"})
      if (!fs::is_regular_file(package / name)) throw std::runtime_error("Incomplete web build.");
    const auto dest = output / "Release";
    const auto archive = output / "Release.zip";
    const auto stagedZip = stage / "Release.zip";
    if (format == "zip") {
      const auto zipCommand = "cmake -E chdir " + q(package) +
                              " cmake -E tar cf " + q(stagedZip) +
                              " --format=zip .";
      if (std::system(zipCommand.c_str()) != 0 || !fs::is_regular_file(stagedZip))
        throw std::runtime_error("ZIP creation failed.");
    }
    struct Publication {
      fs::path source, destination, backup;
      bool saved = false, published = false;
    };
    std::vector<Publication> items{{package, dest, stage / "previous"}};
    if (format == "zip") items.push_back({stagedZip, archive, stage / "previous.zip"});
    for (const auto& item : items)
      if (fs::is_symlink(item.destination)) throw std::runtime_error("Invalid export destination.");
    try {
      for (auto& item : items) {
        if (fs::exists(item.destination)) {
          fs::rename(item.destination, item.backup);
          item.saved = true;
        }
        fs::rename(item.source, item.destination);
        item.published = true;
      }
    } catch (...) {
      for (auto it = items.rbegin(); it != items.rend(); ++it) {
        std::error_code ec;
        if (it->published) fs::rename(it->destination, it->source, ec);
        if (!ec && it->saved) fs::rename(it->backup, it->destination, ec);
        if (ec) {
          cleanup.path.clear();
          std::cerr << "Recovery files retained at " << stage << '\n';
        }
      }
      throw;
    }
    std::cout << "[Levye] HTML exported: " << dest << '\n';
    if (format == "zip") std::cout << "[Levye] ZIP exported: " << archive << '\n';
    return true;
  } catch (const std::exception& e) {
    std::cerr << "[Levye] HTML export failed: " << e.what() << '\n';
    return false;
  }
#endif
}
}  // namespace Levye
