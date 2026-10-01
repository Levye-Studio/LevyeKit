#pragma once

#include <Levye/Core/Services.hpp>

namespace Levye {

/**
 * @brief Public logging interface for reloadable game code.
 *
 * Log forwards messages to the host-owned LevyeKit logger through the
 * currently bound HostServices table.
 */
class Log {
public:
  /**
   * @brief Writes an informational message.
   *
   * @param message Null-terminated message to write.
   */
  static void Info(const char *message) {
    const HostServices *host = Services::Get();

    if (!host || !host->LogInfo || !message) {
      return;
    }

    host->LogInfo(host->context, message);
  }

  /**
   * @brief Writes a warning message.
   *
   * @param message Null-terminated message to write.
   */
  static void Warning(const char *message) {
    const HostServices *host = Services::Get();

    if (!host || !host->LogWarning || !message) {
      return;
    }

    host->LogWarning(host->context, message);
  }

  /**
   * @brief Writes an error message.
   *
   * @param message Null-terminated message to write.
   */
  static void Error(const char *message) {
    const HostServices *host = Services::Get();

    if (!host || !host->LogError || !message) {
      return;
    }

    host->LogError(host->context, message);
  }
};

} // namespace Levye