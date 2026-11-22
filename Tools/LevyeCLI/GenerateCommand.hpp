
#pragma once

#include <filesystem>
#include <string_view>

namespace Levye {

/** @brief Controls overwrite and CMake registration for generated source files. */
struct GenerateOptions {
  bool force = false;
  bool noCMake = false;
};

/** @brief Generates source files under a LevyeKit project's Source directory. */
class GenerateCommand {
 public:
  /** @brief Generates a class, source, header, or folder from validated inputs. */
  static bool Execute(const std::filesystem::path& projectDirectory, std::string_view type,
                      std::string_view name, const GenerateOptions& options = {});
};

}  // namespace Levye
