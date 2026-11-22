#pragma once

#include <filesystem>
#include <string>

#include "BuildConfiguration.hpp"

namespace Levye {

/** @brief Prepares mobile manifest text without writing the project file. */
bool PreparePlatformManifest(const std::filesystem::path& projectDirectory,
                             ProjectPlatform platform, std::string& content, bool& changed);

}  // namespace Levye
