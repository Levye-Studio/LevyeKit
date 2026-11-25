#pragma once
#ifdef _WIN32
#include <Levye/Project/ProjectConfig.hpp>
#include <algorithm>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>

namespace Levye::WindowsPackage {
namespace fs = std::filesystem;
inline std::wstring Lower(std::wstring s) {
  for (auto& c : s)
    c = static_cast<wchar_t>(std::towlower(c));
  return s;
}
inline void Require(bool ok, const char* message) {
  if (!ok)
    throw std::runtime_error(message);
}
inline std::string Architecture(const fs::path& executable) {
  std::ifstream in(executable, std::ios::binary);
  auto read = [&](unsigned count) {
    std::uint32_t n = 0;
    for (unsigned i = 0; i < count; ++i) {
      const int c = in.get();
      Require(c != EOF, "Invalid PE header");
      n |= static_cast<std::uint32_t>(c) << (8 * i);
    }
    return n;
  };
  Require(read(2) == 0x5a4d, "Invalid Windows executable");
  in.seekg(0x3c);
  const auto offset = read(4);
  in.seekg(offset);
  Require(read(4) == 0x4550, "Invalid PE signature");
  switch (read(2)) {
    case 0x8664:
      return "x86_64";
    case 0xaa64:
      return "arm64";
    case 0x14c:
      return "x86";
    default:
      throw std::runtime_error("Unsupported PE architecture");
  }
}
inline void SafeDirectory(const fs::path& path) {
  Require(!fs::is_symlink(fs::symlink_status(path)), "Symlink directory is unsupported");
  if (!fs::exists(path))
    fs::create_directory(path);
  Require(fs::is_directory(path) && fs::canonical(path) == fs::absolute(path).lexically_normal(),
          "Invalid or redirected directory");
}
inline bool Execute(const fs::path& root, const ProjectConfig& project, const fs::path& manifest,
                    const std::string& appName, const std::string& bundleId) {
  const fs::path asset(project.assetDirectory);
  Require(!asset.empty() && !asset.has_root_path(), "Invalid asset path");
  for (const auto& part : asset)
    Require(part != "." && part != "..", "Invalid asset path");
  const auto first = Lower(asset.begin()->wstring());
  Require(first != L"bin" && first != L"lib" && first != L"levye.project" && first != L"exports" &&
              first != L"targets" && first != L"build",
          "Reserved asset path");
  const auto assets = root / asset;
  Require(fs::is_directory(assets) && fs::canonical(assets) == assets, "Invalid asset directory");
  for (const auto& entry : fs::recursive_directory_iterator(assets)) {
    Require(!entry.is_symlink() && fs::canonical(entry.path()) == entry.path(),
            "Redirected asset path");
    Require(entry.is_directory() || entry.is_regular_file(), "Unsupported asset file");
    const auto ext = Lower(entry.path().extension().wstring());
    Require(ext != L".p12" && ext != L".pfx" && ext != L".jks" && ext != L".keystore",
            "Keystore in assets");
  }
  if (!project.desktopIcon.empty()) {
    const fs::path icon(project.desktopIcon);
    Require(!icon.has_root_path(), "Invalid desktop icon");
    for (const auto& part : icon)
      Require(part != ".." && part != ".", "Invalid desktop icon");
    Require(Lower(icon.extension().wstring()) == L".png" && fs::is_regular_file(assets / icon),
            "Missing PNG desktop icon");
  }
  const auto release = root / "Targets/Release";
  const auto host = release / "bin" / (project.target + ".exe");
  const auto game = release / "lib/Game.dll";
  Require(fs::is_regular_file(host) && fs::is_regular_file(game),
          "Missing Release host or game DLL");
  const auto arch = Architecture(host);
  Require(Architecture(game) == arch, "Game DLL architecture mismatch");
  auto out = root / "Exports";
  SafeDirectory(out);
  out /= "Desktop";
  SafeDirectory(out);
  out /= "Windows-" + arch;
  SafeDirectory(out);
  const auto stage = out / ".stage";
  Require(fs::create_directory(stage), "Existing Windows export stage; inspect before retrying");
  struct Cleanup {
    fs::path path;
    bool keep = false;
    ~Cleanup() {
      if (!keep) {
        std::error_code ec;
        fs::remove_all(path, ec);
      }
    }
  } cleanup{stage};
  const auto app = stage / "candidate";
  fs::create_directories(app / "bin");
  fs::create_directories(app / "lib");
  fs::copy_file(host, app / "bin" / host.filename());
  fs::copy_file(game, app / "lib/Game.dll");
  std::map<std::wstring, fs::path> copied;
  // Windows searches the executable directory for both host and module dependencies.
  for (const auto& directory : {release / "bin", release / "lib"}) {
    for (const auto& entry : fs::directory_iterator(directory)) {
      const auto name = Lower(entry.path().filename().wstring());
      if (!entry.is_regular_file() || Lower(entry.path().extension().wstring()) != L".dll" ||
          name == L"game.dll")
        continue;
      Require(Architecture(entry.path()) == arch, "Runtime DLL architecture mismatch");
      if (const auto it = copied.find(name); it != copied.end()) {
        std::ifstream a(it->second, std::ios::binary), b(entry.path(), std::ios::binary);
        Require(fs::file_size(it->second) == fs::file_size(entry.path()) &&
                    std::equal(std::istreambuf_iterator<char>(a), std::istreambuf_iterator<char>(),
                               std::istreambuf_iterator<char>(b)),
                "Conflicting runtime DLLs");
        continue;
      }
      fs::copy_file(entry.path(), app / "bin" / entry.path().filename());
      copied.emplace(name, entry.path());
    }
  }
  Require(copied.count(L"raylib.dll") || copied.count(L"libraylib.dll"),
          "Missing raylib runtime DLL");
  fs::create_directories((app / asset).parent_path());
  fs::copy(assets, app / asset, fs::copy_options::recursive);
  fs::copy_file(manifest, app / "levye.project");
  {
    std::ofstream metadata(app / "lib/desktop-metadata.txt");
    metadata << "Name=" << appName << "\nBundleId=" << bundleId
             << "\nVersion=" << project.desktopVersionName
             << "\nVersionCode=" << project.desktopVersionCode << "\nIcon=" << project.desktopIcon
             << '\n';
    Require(static_cast<bool>(metadata), "Cannot write desktop metadata");
  }
  const auto destination = out / project.target;
  Require(!fs::is_symlink(fs::symlink_status(destination)), "Symlink export destination");
  const auto previous = stage / "previous";
  if (fs::exists(destination)) {
    Require(fs::is_directory(destination) && fs::canonical(destination) == destination,
            "Invalid export destination");
    fs::rename(destination, previous);
  }
  try {
    fs::rename(app, destination);
  } catch (...) {
    if (fs::exists(previous)) {
      std::error_code ec;
      fs::rename(previous, destination, ec);
      if (ec) {
        cleanup.keep = true;
        throw std::runtime_error("Rollback failed; recover previous export from .stage/previous");
      }
    }
    throw;
  }
  std::cout << "[Levye] Desktop exported: " << destination << '\n';
  return true;
}
}  // namespace Levye::WindowsPackage
#endif
