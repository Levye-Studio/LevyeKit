#include "AssetPath.hpp"

#include <filesystem>

namespace Levye {
std::string AssetPath::Normalize(const std::string &path) {
  if (path.empty())
    return {};

  const std::filesystem::path assetPath{path};

  /*
   * lexical_normal() removes redundant path components without touching
   * the filesystem. This keeps asset lookup predictable even before a
   * resource is loaded.
   *
   * generic_string() always uses '/' as the separator, which gives the
   * cache a consistent representation across supported platforms.
   */
  return assetPath.lexically_normal().generic_string();
}

std::string AssetPath::Resolve(const std::string &root,
                               const std::string &path) {
  if (path.empty())
    return {};

  std::filesystem::path assetPath{path};

  /*
   * Absolute paths already identify their location and should not be
   * modified by the project asset root.
   */
  if (assetPath.is_absolute()) {
    return assetPath.lexically_normal().generic_string();
  }

  if (root.empty()) {
    return Normalize(path);
  }

  const std::filesystem::path rootPath{root};

  return (rootPath / assetPath).lexically_normal().generic_string();
}
} // namespace Levye