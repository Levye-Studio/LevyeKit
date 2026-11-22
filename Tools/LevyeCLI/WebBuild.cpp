#include "WebBuild.hpp"

#include <Levye/Platform/ExecutablePath.hpp>
#include <Levye/Project/ProjectLoader.hpp>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <toml++/toml.hpp>

#include "ProcessArguments.hpp"

namespace Levye {
namespace fs = std::filesystem;
fs::path WebBuild::Output(const fs::path& project, BuildConfiguration configuration) {
  return project / "Targets" / ("Web" + std::string(ToString(configuration))) / "bin";
}

bool WebBuild::Execute(const fs::path& directory, BuildConfiguration configuration) {
#if defined(_WIN32)
  std::cerr << "[Levye] Web builds currently require macOS or Linux.\n";
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
        throw std::runtime_error(std::string("Web does not yet support ") + module);

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
    if (!fs::is_directory(source)) throw std::runtime_error("Asset directory is missing.");

    const auto build = root / "Build" / ("Web" + std::string(ToString(configuration)) + "-4.0.20");
    const auto payload = build / "levye-payload";
    const auto output = Output(root, configuration);
    if (fs::is_symlink(root / "Build") || fs::is_symlink(root / "Targets") ||
        fs::is_symlink(build) || fs::is_symlink(payload) || fs::is_symlink(output))
      throw std::runtime_error("Symlinked build/output directories are unsupported.");
    fs::create_directories(build);
    // Recreate the staged payload to remove assets deleted since the last build.
    if (fs::exists(payload)) fs::remove_all(payload);
    fs::create_directories(payload / assets);
    for (const auto& entry : fs::recursive_directory_iterator(source)) {
      if (entry.is_symlink()) throw std::runtime_error("Asset symlinks are unsupported.");
      if (entry.is_directory()) continue;
      if (!entry.is_regular_file()) throw std::runtime_error("Invalid asset file.");
      auto ext = entry.path().extension().string();
      std::transform(ext.begin(), ext.end(), ext.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      if (ext == ".jks" || ext == ".p12" || ext == ".pfx" || ext == ".keystore")
        throw std::runtime_error("Signing keys cannot be embedded in Web builds.");
      const auto destination = payload / assets / entry.path().lexically_relative(source);
      fs::create_directories(destination.parent_path());
      fs::copy_file(entry.path(), destination, fs::copy_options::overwrite_existing);
    }
    if (auto* android = manifest["android"].as_table()) android->erase("signing");
    {
      std::ofstream stream(payload / "levye.project");
      stream << manifest;
      if (!stream) throw std::runtime_error("Cannot write Web manifest.");
    }
    fs::create_directories(output);
    const auto q = [](const fs::path& path) { return LevyeCLI::QuoteArgument(path.string()); };
    const auto sdk = "bash " + q(framework / "Platforms/Web/with-emsdk.sh") + " ";
    const std::string configurationName(ToString(configuration));
    const std::string configure = sdk + "emcmake cmake -S " + q(framework / "Platforms/Web") +
                                  " -B " + q(build) + " -DCMAKE_BUILD_TYPE=" + configurationName +
                                  " " + q("-DLEVYE_ROOT=" + framework.string()) + " " +
                                  q("-DLEVYE_PROJECT_ROOT=" + root.string()) + " " +
                                  q("-DLEVYE_WEB_OUTPUT=" + output.string()) + " " +
                                  q("-DLEVYE_WEB_PAYLOAD=" + payload.string());
    std::cout << "[Levye] Configuring Web " << configurationName << "...\n" << std::flush;
    if (std::system(configure.c_str()) != 0 ||
        std::system(
            (sdk + "cmake --build " + q(build) + " --config " + configurationName).c_str()) != 0)
      return false;
    for (const auto* name : {"index.html", "index.js", "index.wasm", "index.data"})
      if (!fs::is_regular_file(output / name))
        throw std::runtime_error(std::string("Missing Web output: ") + name);
    std::cout << "[Levye] Web " << configurationName << " build: " << output << '\n';
    return true;
  } catch (const std::exception& e) {
    std::cerr << "[Levye] Web build failed: " << e.what() << '\n';
    return false;
  }
#endif
}
}  // namespace Levye
