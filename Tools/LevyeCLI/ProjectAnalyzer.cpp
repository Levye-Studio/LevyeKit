#include "ProjectAnalyzer.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <utility>

namespace Levye {
namespace {
namespace fs = std::filesystem;

struct Command {
  std::string name;
  std::string body;
};

struct Token {
  std::string text;
  std::size_t begin = 0;
  std::size_t end = 0;
};

bool ReadRegular(const fs::path& path, std::string& text) {
  std::error_code ec;
  if (!fs::is_regular_file(fs::symlink_status(path, ec)) || ec) return false;
  std::ifstream input(path, std::ios::binary);
  if (!input) return false;
  text.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
  return !input.bad();
}

bool SafeSource(const fs::path& root, const fs::path& source) {
  if (source.empty() || source.is_absolute()) return false;
  fs::path current = root;
  for (const auto& part : source) {
    if (part == "." || part == ".." || part.empty()) return false;
    current /= part;
    std::error_code ec;
    const auto status = fs::symlink_status(current, ec);
    if (ec || fs::is_symlink(status)) return false;
    if (current == root / source) return fs::is_regular_file(status);
    if (!fs::is_directory(status)) return false;
  }
  return false;
}

bool SafeDirectory(const fs::path& root, const fs::path& directory) {
  if (directory.empty() || directory.is_absolute()) return false;
  fs::path current = root;
  for (const auto& part : directory) {
    if (part == "." || part == ".." || part.empty()) return false;
    current /= part;
    std::error_code ec;
    if (!fs::is_directory(fs::symlink_status(current, ec)) || ec) return false;
  }
  return true;
}

bool SafeName(std::string_view name) {
  if (name.empty() || !(std::isalpha(static_cast<unsigned char>(name[0])) || name[0] == '_'))
    return false;
  for (unsigned char c : name)
    if (!std::isalnum(c) && c != '_' && c != '-' && c != '.' && c != '+') return false;
  return true;
}

bool ParseCommands(const std::string& cmake, std::vector<Command>& commands, std::string& error) {
  std::size_t i = 0;
  while (i < cmake.size()) {
    if (std::isspace(static_cast<unsigned char>(cmake[i]))) {
      ++i;
      continue;
    }
    if (cmake[i] == '#') {
      const auto end = cmake.find('\n', i);
      i = end == std::string::npos ? cmake.size() : end + 1;
      continue;
    }
    const auto begin = i;
    while (i < cmake.size() &&
           (std::isalnum(static_cast<unsigned char>(cmake[i])) || cmake[i] == '_'))
      ++i;
    if (begin == i) {
      error = "Unsupported CMake text outside commands.";
      return false;
    }
    std::string name = cmake.substr(begin, i - begin);
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    while (i < cmake.size() && std::isspace(static_cast<unsigned char>(cmake[i]))) ++i;
    if (i == cmake.size() || cmake[i] != '(') {
      error = "Unsupported CMake command syntax near " + name + ".";
      return false;
    }
    const auto bodyStart = ++i;
    int depth = 1;
    bool quoted = false, escaped = false, comment = false;
    for (; i < cmake.size(); ++i) {
      const char c = cmake[i];
      if (comment) {
        if (c == '\n') comment = false;
        continue;
      }
      if (escaped) {
        escaped = false;
        continue;
      }
      if (c == '\\') {
        escaped = true;
        continue;
      }
      if (c == '"') {
        quoted = !quoted;
        continue;
      }
      if (!quoted && c == '#') {
        comment = true;
        continue;
      }
      if (!quoted && c == '(') ++depth;
      if (!quoted && c == ')' && --depth == 0) break;
    }
    if (i == cmake.size() || quoted || depth != 0) {
      error = "Unbalanced CMake command: " + name + ".";
      return false;
    }
    commands.push_back({std::move(name), cmake.substr(bodyStart, i - bodyStart)});
    ++i;
  }
  return true;
}

bool CMakeArguments(const std::string& body, std::vector<std::string>& arguments) {
  for (std::size_t i = 0; i < body.size();) {
    if (std::isspace(static_cast<unsigned char>(body[i]))) {
      ++i;
      continue;
    }
    if (body[i] == '#') {
      const auto end = body.find('\n', i);
      i = end == std::string::npos ? body.size() : end + 1;
      continue;
    }
    std::string value;
    if (body[i] == '"') {
      ++i;
      while (i < body.size() && body[i] != '"') {
        if (body[i] == '\\') return false;
        value += body[i++];
      }
      if (i == body.size()) return false;
      ++i;
      if (i < body.size() && !std::isspace(static_cast<unsigned char>(body[i])) && body[i] != '#')
        return false;
    } else {
      while (i < body.size() && !std::isspace(static_cast<unsigned char>(body[i])) &&
             body[i] != '#')
        value += body[i++];
    }
    if (value.empty() || value.find(';') != std::string::npos ||
        value.find('*') != std::string::npos || value.find('(') != std::string::npos ||
        value.find(')') != std::string::npos)
      return false;
    arguments.push_back(std::move(value));
  }
  return true;
}

bool IsPlainValue(std::string_view value) {
  if (value.empty()) return false;
  for (unsigned char c : value) {
    if (!std::isalnum(c) && c != '_' && c != '-' && c != '.' && c != '/' && c != '+' && c != '=' &&
        c != ':')
      return false;
  }
  return true;
}

bool ParseCpp(const std::string& source, std::vector<Token>& tokens,
              std::vector<std::string>& directives) {
  bool lineStart = true;
  for (std::size_t i = 0; i < source.size();) {
    const unsigned char c = static_cast<unsigned char>(source[i]);
    if (c == '\n') {
      lineStart = true;
      ++i;
      continue;
    }
    if (std::isspace(c)) {
      ++i;
      continue;
    }
    if (source.compare(i, 2, "//") == 0) {
      const auto end = source.find('\n', i);
      i = end == std::string::npos ? source.size() : end;
      continue;
    }
    if (source.compare(i, 2, "/*") == 0) {
      const auto end = source.find("*/", i + 2);
      if (end == std::string::npos) return false;
      i = end + 2;
      continue;
    }
    if (lineStart && c == '#') {
      const auto end = source.find('\n', i);
      directives.push_back(source.substr(i, end == std::string::npos ? end : end - i));
      i = end == std::string::npos ? source.size() : end;
      continue;
    }
    lineStart = false;
    const auto begin = i;
    if (std::isalpha(c) || c == '_') {
      while (i < source.size() &&
             (std::isalnum(static_cast<unsigned char>(source[i])) || source[i] == '_'))
        ++i;
    } else if (std::isdigit(c)) {
      while (i < source.size() && (std::isalnum(static_cast<unsigned char>(source[i])) ||
                                   source[i] == '.' || source[i] == '_'))
        ++i;
    } else if (c == '"' || c == '\'') {
      const char quote = static_cast<char>(c);
      ++i;
      bool escaped = false, closed = false;
      while (i < source.size()) {
        if (!escaped && source[i] == quote) {
          ++i;
          closed = true;
          break;
        }
        if (!escaped && source[i] == '\\')
          escaped = true;
        else
          escaped = false;
        ++i;
      }
      if (!closed) return false;
    } else {
      ++i;
    }
    tokens.push_back({source.substr(begin, i - begin), begin, i});
  }
  return true;
}

bool IsInteger(const Token& token, int& value) {
  if (token.text.empty() || !std::all_of(token.text.begin(), token.text.end(),
                                         [](unsigned char c) { return std::isdigit(c); }))
    return false;
  try {
    value = std::stoi(token.text);
  } catch (...) {
    return false;
  }
  return true;
}

bool IsNumber(const Token& token) {
  static const std::regex number(R"(^[0-9]+(?:\.[0-9]+)?[fF]?$)");
  return std::regex_match(token.text, number);
}

bool IsColor(std::string_view value) {
  constexpr std::string_view colors[] = {
      "WHITE",  "BLACK",  "RAYWHITE",   "LIGHTGRAY", "GRAY",  "DARKGRAY",
      "YELLOW", "GOLD",   "ORANGE",     "PINK",      "RED",   "MAROON",
      "GREEN",  "LIME",   "DARKGREEN",  "SKYBLUE",   "BLUE",  "DARKBLUE",
      "PURPLE", "VIOLET", "DARKPURPLE", "BEIGE",     "BROWN", "DARKBROWN"};
  return std::find(std::begin(colors), std::end(colors), value) != std::end(colors);
}

bool DrawArguments(std::string_view call, const std::vector<std::vector<Token>>& args) {
  if (call == "ClearBackground")
    return args.size() == 1 && args[0].size() == 1 && IsColor(args[0][0].text);
  if (call == "DrawFPS")
    return args.size() == 2 && args[0].size() == 1 && args[1].size() == 1 && IsNumber(args[0][0]) &&
           IsNumber(args[1][0]);
  std::size_t count = 0;
  bool firstString = false;
  if (call == "DrawText") {
    count = 5;
    firstString = true;
  } else if (call == "DrawRectangle")
    count = 5;
  else if (call == "DrawCircle")
    count = 4;
  else if (call == "DrawLine")
    count = 6;
  else
    return false;
  if (args.size() != count || args.back().size() != 1 || !IsColor(args.back()[0].text))
    return false;
  for (std::size_t i = 0; i + 1 < args.size(); ++i) {
    const auto& arg = args[i];
    if (firstString && i == 0) {
      if (arg.size() != 1 || arg[0].text.size() < 2 || arg[0].text.front() != '"') return false;
    } else {
      const auto offset = arg.size() == 2 && arg[0].text == "-" ? 1u : 0u;
      if (arg.size() != offset + 1 || !IsNumber(arg[offset])) return false;
    }
  }
  return true;
}

bool ParseRaylibMain(const std::string& source, std::string_view projectName,
                     ConversionAnalysis& result, std::string& error) {
  std::vector<Token> tokens;
  std::vector<std::string> directives;
  if (!ParseCpp(source, tokens, directives)) {
    error = "Malformed C++ source.";
    return false;
  }
  const std::regex include(R"(^#\s*include\s*[<"]raylib\.h[>"]\s*$)");
  if (directives.empty() ||
      !std::all_of(directives.begin(), directives.end(),
                   [&](const std::string& d) { return std::regex_match(d, include); })) {
    error = "Automatic raylib conversion supports only a raylib.h include in the entry file.";
    return false;
  }
  std::size_t i = 0;
  auto take = [&](std::string_view expected) {
    if (i >= tokens.size() || tokens[i].text != expected) return false;
    ++i;
    return true;
  };
  if (!take("int") || !take("main") || !take("(")) {
    error = "Expected a standalone int main() entry point.";
    return false;
  }
  if (i < tokens.size() && tokens[i].text == "void") ++i;
  if (!take(")") || !take("{") || !take("InitWindow") || !take("(")) {
    error = "Expected InitWindow at the start of main().";
    return false;
  }
  if (i >= tokens.size() || !IsInteger(tokens[i++], result.windowWidth) || !take(",") ||
      i >= tokens.size() || !IsInteger(tokens[i++], result.windowHeight) || !take(",") ||
      i >= tokens.size()) {
    error = "InitWindow must use positive literal dimensions.";
    return false;
  }
  const auto title = tokens[i++].text;
  if (title != "\"" + std::string(projectName) + "\"" || result.windowWidth <= 0 ||
      result.windowHeight <= 0 || !take(")") || !take(";")) {
    error = "InitWindow title must match the CMake project name and use literal dimensions.";
    return false;
  }
  if (!take("SetTargetFPS") || !take("(") || i >= tokens.size() ||
      !IsInteger(tokens[i++], result.targetFPS) || result.targetFPS <= 0 || !take(")") ||
      !take(";")) {
    error = "Expected SetTargetFPS with a positive literal frame rate.";
    return false;
  }
  if (!take("while") || !take("(") || !take("!") || !take("WindowShouldClose") || !take("(") ||
      !take(")") || !take(")") || !take("{") || !take("BeginDrawing") || !take("(") || !take(")") ||
      !take(";")) {
    error = "Expected the standard WindowShouldClose/BeginDrawing loop.";
    return false;
  }
  const auto drawStart = i < tokens.size() ? tokens[i].begin : source.size();
  int drawCount = 0;
  while (i < tokens.size() && tokens[i].text != "EndDrawing") {
    const auto call = tokens[i++].text;
    if (!take("(")) {
      error = "Drawing loop contains an unsupported statement.";
      return false;
    }
    std::vector<std::vector<Token>> arguments(1);
    while (i < tokens.size() && tokens[i].text != ")") {
      if (tokens[i].text == ",")
        arguments.emplace_back();
      else
        arguments.back().push_back(tokens[i]);
      ++i;
    }
    if (!take(")") || !take(";") || !DrawArguments(call, arguments)) {
      error = "Drawing loop contains an unsupported call or expression: " + call;
      return false;
    }
    ++drawCount;
  }
  if (drawCount == 0 || i == tokens.size()) {
    error = "Drawing loop must contain supported raylib drawing calls.";
    return false;
  }
  const auto drawEnd = tokens[i].begin;
  result.drawCalls = source.substr(drawStart, drawEnd - drawStart);
  if (!take("EndDrawing") || !take("(") || !take(")") || !take(";") || !take("}") ||
      !take("CloseWindow") || !take("(") || !take(")") || !take(";")) {
    error = "Expected EndDrawing and CloseWindow in the standard lifecycle.";
    return false;
  }
  if (i < tokens.size() && tokens[i].text == "return") {
    ++i;
    if (!take("0") || !take(";")) {
      error = "Only return 0 is supported after CloseWindow.";
      return false;
    }
  }
  if (!take("}") || i != tokens.size()) {
    error = "Entry file has additional statements outside the supported main().";
    return false;
  }
  return true;
}

int CountMains(const std::string& source) {
  std::vector<Token> tokens;
  std::vector<std::string> directives;
  if (!ParseCpp(source, tokens, directives)) return -1;
  int count = 0;
  for (std::size_t i = 1; i + 1 < tokens.size(); ++i)
    if (tokens[i].text == "main" && tokens[i + 1].text == "(" &&
        (tokens[i - 1].text == "int" || tokens[i - 1].text == "auto"))
      ++count;
  return count;
}
}  // namespace

ConversionAnalysis ProjectAnalyzer::Analyze(const fs::path& projectRoot) {
  ConversionAnalysis result;
  std::error_code ec;
  const auto status = fs::symlink_status(projectRoot, ec);
  if (ec || !fs::is_directory(status)) {
    result.errors.push_back("Project root is missing, a symlink, or not a directory.");
    return result;
  }
  result.projectRoot = fs::canonical(projectRoot, ec);
  if (ec) {
    result.errors.push_back("Cannot resolve project root.");
    return result;
  }
  const auto marker = result.projectRoot / "levye.project";
  if (fs::exists(marker, ec)) {
    result.alreadyLevyeKit = true;
    result.errors.push_back("Already a LevyeKit project; use levye upgrade.");
    return result;
  }
  if (ec || !ReadRegular(result.projectRoot / "CMakeLists.txt", result.originalCMake)) {
    result.errors.push_back("Missing or unsafe CMakeLists.txt.");
    return result;
  }
  std::vector<Command> commands;
  std::string error;
  if (!ParseCommands(result.originalCMake, commands, error)) {
    result.errors.push_back(error);
    return result;
  }
  bool standard20 = false;
  for (const auto& command : commands) {
    std::vector<std::string> args;
    if (!CMakeArguments(command.body, args)) {
      result.errors.push_back("Unsupported arguments in CMake command " + command.name + ".");
      continue;
    }
    if (command.name == "cmake_minimum_required") {
      if (args.size() != 2 || args[0] != "VERSION")
        result.errors.push_back("Unsupported CMake version declaration.");
    } else if (command.name == "project") {
      if (args.size() != 3 || args[1] != "LANGUAGES" || args[2] != "CXX" || !SafeName(args[0]) ||
          !result.projectName.empty())
        result.errors.push_back("Expected one literal C++ project(name LANGUAGES CXX).");
      else
        result.projectName = args[0];
    } else if (command.name == "set") {
      if (args.size() != 2 ||
          (args[0] != "CMAKE_CXX_STANDARD" && args[0] != "CMAKE_CXX_STANDARD_REQUIRED" &&
           args[0] != "CMAKE_EXPORT_COMPILE_COMMANDS")) {
        result.errors.push_back("Unsupported CMake set() expression.");
      } else if (args[0] == "CMAKE_CXX_STANDARD") {
        standard20 = args[1] == "20";
      }
    } else if (command.name == "find_package") {
      if (!((args.size() == 2 && args[0] == "raylib" && args[1] == "REQUIRED") ||
            (args.size() == 3 && args[0] == "raylib" && args[1] == "6.2" && args[2] == "REQUIRED")))
        result.errors.push_back("External package dependencies require manual conversion.");
    } else if (command.name == "add_executable" || command.name == "add_library") {
      if (args.size() < 2) {
        result.errors.push_back("Target has no literal source files.");
        continue;
      }
      ConversionTarget target;
      target.name = args[0];
      if (!SafeName(target.name))
        result.errors.push_back("Unsafe or unsupported target name: " + target.name);
      std::size_t sourceStart = 1;
      if (command.name == "add_executable")
        target.type = ConversionProjectType::Executable;
      else {
        if (args[1] == "STATIC")
          target.type = ConversionProjectType::StaticLibrary;
        else if (args[1] == "SHARED")
          target.type = ConversionProjectType::SharedLibrary;
        else
          result.errors.push_back("Only STATIC or SHARED libraries are supported.");
        sourceStart = 2;
      }
      for (std::size_t i = sourceStart; i < args.size(); ++i) {
        const fs::path source(args[i]);
        if (args[i].find('$') != std::string::npos || args[i].find('\n') != std::string::npos ||
            args[i].find('\r') != std::string::npos ||
            (source.extension() != ".cpp" && source.extension() != ".cc" &&
             source.extension() != ".cxx") ||
            !SafeSource(result.projectRoot, source))
          result.errors.push_back("Missing, generated, or escaping source: " + args[i]);
        else
          target.sources.push_back(source);
      }
      if (target.sources.empty()) result.errors.push_back("Target has no supported C++ sources.");
      result.targets.push_back(std::move(target));
    } else if (command.name == "target_link_libraries" ||
               command.name == "target_include_directories" ||
               command.name == "target_compile_definitions" ||
               command.name == "target_compile_options") {
      if (args.size() < 3 || result.targets.size() != 1 || args[0] != result.targets[0].name ||
          (args[1] != "PRIVATE" && args[1] != "PUBLIC" && args[1] != "INTERFACE")) {
        result.errors.push_back("Unsupported target settings or dependency layout.");
        continue;
      }
      auto& target = result.targets[0];
      target.cmakeSettings.push_back(command.name + "(" + command.body + ")");
      for (std::size_t i = 2; i < args.size(); ++i) {
        if (command.name == "target_link_libraries") {
          if (args[i] != "raylib")
            result.errors.push_back("External dependency requires manual conversion: " + args[i]);
          else
            target.dependencies.push_back(args[i]);
        } else if (command.name == "target_include_directories") {
          std::string path = args[i];
          constexpr std::string_view prefix = "${CMAKE_CURRENT_SOURCE_DIR}/";
          if (path.starts_with(prefix)) path.erase(0, prefix.size());
          if (path.find('$') != std::string::npos || !SafeDirectory(result.projectRoot, path))
            result.errors.push_back("Unsupported include directory: " + args[i]);
          else
            target.includeDirectories.push_back(path);
        } else if (!IsPlainValue(args[i])) {
          result.errors.push_back("Unsupported compile setting: " + args[i]);
        } else if (command.name == "target_compile_definitions") {
          target.compileDefinitions.push_back(args[i]);
        } else {
          target.compileOptions.push_back(args[i]);
        }
      }
    } else {
      result.errors.push_back("Unsupported CMake command: " + command.name + ".");
    }
  }
  if (!standard20) result.errors.push_back("Supported projects must explicitly use C++20.");
  if (result.projectName.empty()) result.errors.push_back("No supported CMake project name found.");
  if (result.targets.size() != 1)
    result.errors.push_back("Expected exactly one executable or library target.");
  if (!result.errors.empty()) return result;

  auto& target = result.targets.front();
  if (target.name == "Game")
    result.errors.push_back("Original target name Game conflicts with the game module.");
  int mainCount = 0;
  for (const auto& source : target.sources) {
    std::string code;
    if (!ReadRegular(result.projectRoot / source, code)) {
      result.errors.push_back("Cannot read source: " + source.generic_string());
      continue;
    }
    if (target.type == ConversionProjectType::Executable) {
      if (code.find("SDL_Init(") != std::string::npos ||
          code.find("SDL_CreateWindow(") != std::string::npos)
        result.lifecycle = ConversionLifecycle::SDLApplication;
      else if (code.find("glfwInit(") != std::string::npos ||
               code.find("glfwCreateWindow(") != std::string::npos)
        result.lifecycle = ConversionLifecycle::GLFWApplication;
    }
    const int count = CountMains(code);
    if (count < 0)
      result.errors.push_back("Malformed C++ source: " + source.generic_string());
    else if (count > 0) {
      mainCount += count;
      result.entryPoint = source;
    }
  }
  if (mainCount > 1) result.errors.push_back("Multiple main() definitions are unsupported.");
  if (target.type == ConversionProjectType::Executable) {
    if (mainCount == 0)
      result.errors.push_back("Executable has no main() definition.");
    else if (target.dependencies.size() != 1 || target.dependencies[0] != "raylib")
      result.errors.push_back(
          result.lifecycle == ConversionLifecycle::SDLApplication ||
                  result.lifecycle == ConversionLifecycle::GLFWApplication
              ? "SDL/GLFW lifecycle requires manual adaptation."
              : "Ordinary C++ executable lifecycle requires manual adaptation.");
    else if (target.sources.size() != 1)
      result.errors.push_back(
          "Automatic raylib lifecycle adaptation currently requires one source file.");
    else if (result.errors.empty()) {
      std::string code;
      ReadRegular(result.projectRoot / result.entryPoint, code);
      if (!ParseRaylibMain(code, result.projectName, result, error))
        result.errors.push_back(error);
      else
        result.lifecycle = ConversionLifecycle::RaylibApplication;
    }
  } else {
    if (mainCount != 0) result.errors.push_back("Library contains a main() entry point.");
    result.lifecycle = ConversionLifecycle::NoEntryPoint;
  }
  for (const char* directory : {"Assets", "assets", "Resources", "resources"}) {
    fs::path candidate;
    for (const auto& entry : fs::directory_iterator(result.projectRoot, ec)) {
      if (entry.path().filename() == directory) {
        candidate = entry.path();
        break;
      }
    }
    if (ec) {
      result.errors.push_back("Cannot inspect project asset directories.");
      break;
    }
    if (candidate.empty()) continue;
    ec.clear();
    const auto fileStatus = fs::symlink_status(candidate, ec);
    if (fileStatus.type() == fs::file_type::not_found ||
        ec == std::errc::no_such_file_or_directory) {
      ec.clear();
      continue;
    }
    if (ec) {
      result.errors.push_back("Cannot inspect asset path: " + std::string(directory));
      continue;
    }
    if (!ec && fs::is_symlink(fileStatus))
      result.errors.push_back("Asset path is a symlink: " + std::string(directory));
    if (!ec && !fs::is_directory(fileStatus))
      result.errors.push_back("Asset path is not a directory: " + std::string(directory));
    if (!ec && fs::is_directory(fileStatus)) {
      result.assetDirectory = directory;
      break;
    }
  }
  return result;
}

}  // namespace Levye
