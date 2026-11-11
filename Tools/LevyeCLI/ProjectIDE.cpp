
#include "ProjectIDE.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;

namespace Levye {

bool ProjectIDE::Prepare(const fs::path& projectDirectory, const fs::path& templateDirectory,
                         std::string_view target, std::vector<PendingFile>& changes,
                         std::vector<fs::path>& preserved, std::string& error) {
  constexpr std::array<const char*, 5> files = {".clangd", ".vscode/settings.json",
                                                ".vscode/tasks.json", ".vscode/extensions.json",
                                                ".vscode/launch.json"};
  for (const char* name : files) {
    const auto source = templateDirectory / name;
    std::error_code ec;
    if (!fs::is_regular_file(source, ec) || ec) {
      error = "Missing IDE template: " + source.string();
      return false;
    }
    const auto destination = projectDirectory / name;
    const auto status = fs::symlink_status(destination, ec);
    if (ec && ec != std::errc::no_such_file_or_directory) {
      error = "Cannot inspect IDE file: " + destination.string();
      return false;
    }
    if (!ec && fs::exists(status)) {
      if (!fs::is_regular_file(status)) {
        error = "Unsafe IDE file: " + destination.string();
        return false;
      }
      preserved.emplace_back(name);
      continue;
    }
    std::ifstream input(source, std::ios::binary);
    if (!input) {
      error = "Cannot read IDE template: " + source.string();
      return false;
    }
    std::string content{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    if (input.bad()) {
      error = "Cannot read IDE template: " + source.string();
      return false;
    }
    if (std::string_view(name) == ".vscode/launch.json") {
      constexpr std::string_view placeholder = "${workspaceFolderBasename}";
      const auto at = content.find(placeholder);
      if (at == std::string::npos) {
        error = "IDE launch template is missing its target placeholder.";
        return false;
      }
      content.replace(at, placeholder.size(), target);
    }
    changes.push_back({destination, std::move(content), false});
  }
  return true;
}

}  // namespace Levye
