
#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace Levye {

struct PendingFile {
  std::filesystem::path path;
  std::string content;
  bool allowReplace = false;
};

// Used by regression tests only.
// Index is zero-based.
enum class TransactionFailurePoint { None, BeforeBackup, AfterBackup, BeforeInstall, AfterInstall };

struct TransactionFailureInjection {
  TransactionFailurePoint point = TransactionFailurePoint::None;
  std::size_t fileIndex = 0;
};

// Stages all outputs, installs them and rolls back on failure.
// Not crash-atomic. Concurrent writers are unsupported.
bool CommitFiles(const std::filesystem::path& projectRoot, const std::vector<PendingFile>& files,
                 std::string& error, const TransactionFailureInjection* injection = nullptr,
                 const std::function<bool(std::string&)>& validate = {});

}  // namespace Levye
