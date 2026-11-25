#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "FileTransaction.hpp"

namespace Levye {

/** @brief Persists the pre-conversion files and an output fingerprint manifest. */
bool SaveConversionBackup(const std::filesystem::path& root, const std::vector<PendingFile>& files,
                          std::filesystem::path& backup, std::string& error);

/** @brief Restores a conversion only when its generated files are unchanged. */
bool RollbackConversion(const std::filesystem::path& root, std::string& error);

}  // namespace Levye
