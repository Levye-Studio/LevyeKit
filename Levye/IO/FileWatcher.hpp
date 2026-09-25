#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>

namespace Levye {
/**
 * @brief Watches one file for stable changes.
 *
 * FileWatcher tracks modification time and file size. A change is reported
 * only after the file has remained unchanged for the debounce period.
 *
 * This prevents reload systems from reading a file while another process
 * is still writing or replacing it.
 */
class FileWatcher {
public:
  /**
   * @brief Creates an empty file watcher.
   */
  FileWatcher() = default;

  /**
   * @brief Creates a watcher for a file.
   *
   * @param path File to watch.
   * @param debounce Minimum time a detected change must remain stable.
   */
  explicit FileWatcher(
      const std::string &path,
      std::chrono::milliseconds debounce = std::chrono::milliseconds{200});

  /**
   * @brief Begins watching a file.
   *
   * The current file state becomes the initial known-good state and does
   * not count as a change.
   *
   * @param path File to watch.
   * @return true if the file state could be read successfully.
   */
  bool Watch(const std::string &path);

  /**
   * @brief Checks whether the watched file has changed and stabilized.
   *
   * @return true once for each stable file change.
   */
  bool Poll();

  /**
   * @brief Returns the path currently being watched.
   */
  const std::string &GetPath() const;

  /**
   * @brief Returns whether this watcher has a valid file to monitor.
   */
  bool IsWatching() const;

private:
  struct FileState {
    std::filesystem::file_time_type writeTime{};
    std::uintmax_t size = 0;

    bool operator==(const FileState &) const = default;
  };

  /**
   * @brief Reads the current state of the watched file.
   */
  bool ReadState(FileState &state) const;

  std::string m_Path;

  FileState m_AcceptedState{};
  FileState m_PendingState{};

  bool m_HasAcceptedState = false;
  bool m_HasPendingChange = false;

  std::chrono::steady_clock::time_point m_ChangeDetectedAt{};

  std::chrono::milliseconds m_Debounce{200};
};
} // namespace Levye