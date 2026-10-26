#pragma once

#include <string>

namespace LevyeCLI {

// Quote a single argument for the platform's command shell.
// Use this only when constructing commands for std::system().
inline std::string QuoteArgument(const std::string& value) {
#ifdef _WIN32
  std::string result = "\"";

  std::size_t backslashes = 0;

  for (char c : value) {
    if (c == '\\') {
      ++backslashes;
      continue;
    }

    if (c == '"') {
      result.append(backslashes * 2 + 1, '\\');
      result += '"';
      backslashes = 0;
      continue;
    }

    result.append(backslashes, '\\');
    backslashes = 0;
    result += c;
  }

  result.append(backslashes * 2, '\\');
  result += '"';

  return result;
#else
  std::string result = "'";

  for (char c : value) {
    if (c == '\'')
      result += "'\\''";
    else
      result += c;
  }

  result += '\'';
  return result;
#endif
}

}  // namespace LevyeCLI
