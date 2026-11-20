#pragma once
#include <filesystem>
#include <string_view>
namespace Levye {
struct WebExporter {
  static bool Execute(const std::filesystem::path& projectDirectory, std::string_view format);
};
}  // namespace Levye
