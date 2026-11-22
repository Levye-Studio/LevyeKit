
#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace Levye {

/** @brief File content to create or replace in a migration transaction. */
struct PendingFile {
  std::filesystem::path path;
  std::string content;
  bool allowReplace = false;
};

/** @brief Regression-only transaction failure point; file indices are zero based. */
enum class TransactionFailurePoint { None, BeforeBackup, AfterBackup, BeforeInstall, AfterInstall };

/** @brief Injects a failure at a selected file and transaction stage in tests. */
struct TransactionFailureInjection {
  TransactionFailurePoint point = TransactionFailurePoint::None;
  std::size_t fileIndex = 0;
};

/** @brief Stages files, installs them, and rolls back recoverable failures.
 * @param projectRoot Root restricting destination paths.
 * @param files Planned file writes.
 * @param error Receives a diagnostic when the transaction fails.
 * @param injection Optional regression-only failure injection.
 * @param validate Optional callback run after installation, before completion.
 * @return True when all files are installed and validation succeeds.
 * This operation is not crash atomic; concurrent writers are unsupported.
 */
bool CommitFiles(const std::filesystem::path& projectRoot, const std::vector<PendingFile>& files,
                 std::string& error, const TransactionFailureInjection* injection = nullptr,
                 const std::function<bool(std::string&)>& validate = {});

}  // namespace Levye
