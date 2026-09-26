#pragma once

#include <filesystem>

namespace Levye {
/**
 * @brief Provides filesystem information about the running executable.
 */
class ExecutablePath {
public:
  /**
   * @brief Returns the absolute path to the running executable.
   *
   * @return Absolute filesystem path to the current executable.
   */
  static std::filesystem::path Get();

  /**
   * @brief Returns the directory containing the running executable.
   *
   * @return Absolute filesystem path to the executable directory.
   */
  static std::filesystem::path GetDirectory();
};
} // namespace Levye