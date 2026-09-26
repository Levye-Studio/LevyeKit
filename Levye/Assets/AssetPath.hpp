#pragma once

#include <string>

namespace Levye {
/**
 * @brief Utilities for converting asset paths into stable cache keys.
 *
 * Resource managers use normalized paths so equivalent filesystem paths
 * resolve to the same host-owned asset.
 */
class AssetPath {
public:
  /**
   * @brief Normalizes an asset path for use as a resource cache key.
   *
   * The returned path is lexically normalized and uses generic path
   * separators. The function does not require the file to exist.
   *
   * @param path Asset path to normalize.
   * @return Normalized path string, or an empty string when path is empty.
   */
  static std::string Normalize(const std::string &path);

  /**
   * @brief Resolves an asset path relative to a configured asset root.
   *
   * Absolute paths are preserved. Relative paths are joined to the asset root
   * before being normalized.
   *
   * @param root Root directory containing project assets.
   * @param path Asset path supplied by game code.
   * @return Resolved normalized filesystem path.
   */
  static std::string Resolve(const std::string &root, const std::string &path);
};
} // namespace Levye