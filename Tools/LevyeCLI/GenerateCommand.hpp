
#pragma once

#include <filesystem>
#include <string_view>

namespace Levye {

struct GenerateOptions {
  bool force = false;
  bool noCMake = false;
};

class GenerateCommand {
 public:
  static bool Execute(const std::filesystem::path& projectDirectory, std::string_view type,
                      std::string_view name, const GenerateOptions& options = {});
};

}  // namespace Levye
