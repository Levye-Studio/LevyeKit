#pragma once
#include <filesystem>
#include <string_view>
namespace Levye {
/** @brief Builds and packages the experimental HTML export. */
struct WebExporter {
  /** @brief Produces an HTML package and optional ZIP archive. */
  static bool Execute(const std::filesystem::path& projectDirectory, std::string_view format);
};
}  // namespace Levye
