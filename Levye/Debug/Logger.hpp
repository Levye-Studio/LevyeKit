#pragma once

#include "LogLevel.hpp"

#include <string_view>

namespace Levye {

/**
 * @brief Provides centralized runtime logging for LevyeKit.
 *
 * Logger formats messages consistently and allows the framework to control
 * which severity levels are written to the console.
 */
class Logger {
public:
  /**
   * @brief Writes a message using the specified severity level.
   *
   * Messages below the current minimum log level are ignored.
   *
   * @param level Severity of the message.
   * @param message Message to write.
   */
  static void Log(LogLevel level, std::string_view message);

  /**
   * @brief Sets the minimum severity that will be written.
   *
   * @param level Minimum visible log level.
   */
  static void SetLevel(LogLevel level);

  /**
   * @brief Returns the current minimum logging severity.
   *
   * @return Current minimum log level.
   */
  static LogLevel GetLevel();

  /**
   * @brief Writes a trace-level message.
   */
  static void Trace(std::string_view message);

  /**
   * @brief Writes a debug-level message.
   */
  static void Debug(std::string_view message);

  /**
   * @brief Writes an informational message.
   */
  static void Info(std::string_view message);

  /**
   * @brief Writes a warning message.
   */
  static void Warning(std::string_view message);

  /**
   * @brief Writes an error message.
   */
  static void Error(std::string_view message);

private:
  static LogLevel s_Level;
};

} // namespace Levye