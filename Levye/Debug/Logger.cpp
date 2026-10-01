#include "Logger.hpp"

#include <iostream>

namespace {

const char *GetLevelName(Levye::LogLevel level) {
  switch (level) {
  case Levye::LogLevel::Trace:
    return "Trace";

  case Levye::LogLevel::Debug:
    return "Debug";

  case Levye::LogLevel::Info:
    return "Info";

  case Levye::LogLevel::Warning:
    return "Warning";

  case Levye::LogLevel::Error:
    return "Error";
  }

  return "Unknown";
}

} // namespace

namespace Levye {

LogLevel Logger::s_Level = LogLevel::Info;

void Logger::Log(LogLevel level, std::string_view message) {
  if (static_cast<int>(level) < static_cast<int>(s_Level)) {
    return;
  }

  std::ostream &output = level == LogLevel::Error ? std::cerr : std::cout;

  output << "[LevyeKit] [" << GetLevelName(level) << "] " << message << '\n';
}

void Logger::SetLevel(LogLevel level) { s_Level = level; }

LogLevel Logger::GetLevel() { return s_Level; }

void Logger::Trace(std::string_view message) { Log(LogLevel::Trace, message); }

void Logger::Debug(std::string_view message) { Log(LogLevel::Debug, message); }

void Logger::Info(std::string_view message) { Log(LogLevel::Info, message); }

void Logger::Warning(std::string_view message) {
  Log(LogLevel::Warning, message);
}

void Logger::Error(std::string_view message) { Log(LogLevel::Error, message); }

} // namespace Levye