#include "FileWatcher.hpp"

namespace Levye {
FileWatcher::FileWatcher(const std::string &path,
                         std::chrono::milliseconds debounce)
    : m_Debounce(debounce) {
  Watch(path);
}

bool FileWatcher::Watch(const std::string &path) {
  m_Path = path;
  m_HasAcceptedState = false;
  m_HasPendingChange = false;

  FileState state{};

  if (!ReadState(state))
    return false;

  m_AcceptedState = state;
  m_HasAcceptedState = true;

  return true;
}

bool FileWatcher::Poll() {
  if (m_Path.empty())
    return false;

  FileState currentState{};

  /*
   * A temporarily missing file is not treated as a completed change.
   * Some editors replace files using remove/rename operations.
   */
  if (!ReadState(currentState))
    return false;

  if (!m_HasAcceptedState) {
    m_AcceptedState = currentState;
    m_HasAcceptedState = true;

    return false;
  }

  if (currentState == m_AcceptedState) {
    m_HasPendingChange = false;

    return false;
  }

  const auto now = std::chrono::steady_clock::now();

  /*
   * We either found a new change or the file changed again while waiting.
   * Restart the debounce timer.
   */
  if (!m_HasPendingChange || currentState != m_PendingState) {
    m_PendingState = currentState;
    m_ChangeDetectedAt = now;
    m_HasPendingChange = true;

    return false;
  }

  if (now - m_ChangeDetectedAt < m_Debounce)
    return false;

  /*
   * The changed state remained stable for the full debounce period.
   * Accept it and report exactly one change.
   */
  m_AcceptedState = currentState;
  m_HasPendingChange = false;

  return true;
}

const std::string &FileWatcher::GetPath() const { return m_Path; }

bool FileWatcher::IsWatching() const {
  return !m_Path.empty() && m_HasAcceptedState;
}

bool FileWatcher::ReadState(FileState &state) const {
  if (m_Path.empty())
    return false;

  std::error_code existsError;

  if (!std::filesystem::exists(m_Path, existsError) || existsError) {
    return false;
  }

  std::error_code timeError;
  std::error_code sizeError;

  state.writeTime = std::filesystem::last_write_time(m_Path, timeError);

  state.size = std::filesystem::file_size(m_Path, sizeError);

  return !timeError && !sizeError;
}
} // namespace Levye