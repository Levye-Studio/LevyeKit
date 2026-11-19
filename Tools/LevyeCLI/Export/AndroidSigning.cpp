#include "AndroidSigning.hpp"

#include <Levye/Project/ProjectLoader.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <random>
#include <regex>
#include <string>
#include <string_view>
#include <toml++/toml.hpp>

#include "../FileTransaction.hpp"
#include "../ProcessArguments.hpp"

#ifdef _WIN32
#include <io.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace Levye {
namespace {

namespace fs = std::filesystem;

bool IsInteractive() {
#ifdef _WIN32
  return _isatty(_fileno(stdin)) && _isatty(_fileno(stdout));
#else
  return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
#endif
}

std::optional<std::string> Prompt(const std::string& label, const std::string& fallback = "") {
  std::cout << label;

  if (!fallback.empty()) {
    std::cout << " [" << fallback << "]";
  }

  std::cout << ": " << std::flush;

  std::string value;

  if (!std::getline(std::cin, value)) {
    return std::nullopt;
  }

  return value.empty() ? fallback : value;
}

fs::path GetSigningDirectory() {
#ifdef _WIN32
  const char* home = std::getenv("USERPROFILE");
#else
  const char* home = std::getenv("HOME");
#endif

  if (!home || !*home) {
    return {};
  }

  return fs::path(home) / ".levye" / "signing";
}

bool IsValidAlias(const std::string& alias) {
  static const std::regex pattern(R"([A-Za-z0-9_.-]+)");

  return std::regex_match(alias, pattern);
}

bool IsValidPackage(const std::string& package) {
  static const std::regex pattern(R"([A-Za-z][A-Za-z0-9_]*(\.[A-Za-z][A-Za-z0-9_]*)+)");

  return std::regex_match(package, pattern);
}

bool GenerateKeystore(const fs::path& keystore, const std::string& alias) {
  std::error_code error;

#ifndef _WIN32
  // Restrict both the new directory and key file to the current user.
  const auto previousMask = umask(0077);
#endif
  fs::create_directories(keystore.parent_path(), error);

  if (error) {
#ifndef _WIN32
    umask(previousMask);
#endif
    std::cerr << "[Levye] Cannot create signing directory: " << error.message() << '\n';
    return false;
  }

  if (fs::exists(keystore)) {
#ifndef _WIN32
    umask(previousMask);
#endif
    std::cerr << "[Levye] Keystore already exists.\n";
    return false;
  }

  fs::path staged;
  std::random_device random;
  for (int attempt = 0; attempt < 32; ++attempt) {
    staged = fs::path(keystore.string() + ".tmp." + std::to_string(random()));
    if (!fs::exists(staged)) break;
    staged.clear();
  }
  if (staged.empty()) {
#ifndef _WIN32
    umask(previousMask);
#endif
    std::cerr << "[Levye] Cannot allocate a temporary keystore path.\n";
    return false;
  }

  const std::string command =
      "keytool -genkeypair"
      " -storetype PKCS12"
      " -keyalg RSA"
      " -keysize 3072"
      " -validity 10000"
      " -keystore " +
      LevyeCLI::QuoteArgument(staged.string()) + " -alias " + LevyeCLI::QuoteArgument(alias);

  const bool success = std::system(command.c_str()) == 0;

#ifndef _WIN32
  umask(previousMask);
#endif

  if (!success || !fs::is_regular_file(staged)) {
    fs::remove(staged, error);
    std::cerr << "[Levye] Keystore generation failed.\n";
    return false;
  }

  // A hard link installs the new key only when the destination is absent.
  fs::create_hard_link(staged, keystore, error);
  if (!error) fs::remove(staged, error);
  if (error) {
    std::cerr << "[Levye] Cannot publish new keystore; generated key remains at: " << staged
              << '\n';
    return false;
  }

  return true;
}

bool VerifyKeystore(const fs::path& keystore, const std::string& alias) {
  const std::string command = "keytool -list -keystore " +
                              LevyeCLI::QuoteArgument(keystore.string()) + " -alias " +
                              LevyeCLI::QuoteArgument(alias);

  if (std::system(command.c_str()) != 0) {
    std::cerr << "[Levye] Cannot verify keystore alias.\n";
    return false;
  }

  return true;
}

bool SaveConfiguration(const fs::path& projectDirectory, const fs::path& keystore,
                       const std::string& alias) {
  const auto manifestPath = projectDirectory / "levye.project";

  std::ifstream input(manifestPath, std::ios::binary);
  if (!input) {
    std::cerr << "[Levye] Cannot read project manifest.\n";
    return false;
  }
  std::string content{std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
  if (input.bad()) return false;

  toml::table manifest;
  try {
    manifest = toml::parse(content);
  } catch (const toml::parse_error& error) {
    std::cerr << "[Levye] Invalid project manifest: " << error << '\n';
    return false;
  }

  auto* android = manifest["android"].as_table();
  if (!android || android->contains("signing")) {
    std::cerr << "[Levye] Expected [android] without existing signing settings.\n";
    return false;
  }

  const auto quoteToml = [](std::string_view value) {
    std::string result = "\"";
    for (unsigned char character : value) {
      switch (character) {
        case '\\':
          result += "\\\\";
          break;
        case '"':
          result += "\\\"";
          break;
        case '\n':
          result += "\\n";
          break;
        case '\r':
          result += "\\r";
          break;
        case '\t':
          result += "\\t";
          break;
        default:
          if (character < 0x20 || character == 0x7f) {
            constexpr char hex[] = "0123456789ABCDEF";
            result += "\\u00";
            result += hex[character >> 4];
            result += hex[character & 15];
          } else {
            result += static_cast<char>(character);
          }
      }
    }
    return result + '"';
  };

  if (!content.empty() && content.back() != '\n') content += '\n';
  content += "\n[android.signing]\nkeystore = " + quoteToml(keystore.generic_string()) +
             "\nalias = " + quoteToml(alias) + '\n';

  std::string error;

  const bool committed =
      CommitFiles(projectDirectory, {{manifestPath, content, true}}, error, nullptr,
                  [&](std::string& validationError) {
                    const auto updated = ProjectLoader::Load(manifestPath);

                    if (updated && updated->androidSigningKeystore == keystore.generic_string() &&
                        updated->androidSigningAlias == alias) {
                      return true;
                    }

                    validationError = "Signing configuration validation failed.";
                    return false;
                  });

  if (!committed) {
    std::cerr << "[Levye] Cannot save signing configuration: " << error << '\n';
    return false;
  }

  return true;
}

}  // namespace

bool AndroidSigning::EnsureConfigured(const fs::path& projectDirectory) {
  const auto project = ProjectLoader::Load(projectDirectory / "levye.project");

  if (!project) {
    return false;
  }

  const bool hasKeystore = !project->androidSigningKeystore.empty();

  const bool hasAlias = !project->androidSigningAlias.empty();

  // Never replace an existing signing identity automatically.
  if (hasKeystore && hasAlias) {
    if (!IsValidAlias(project->androidSigningAlias)) {
      std::cerr << "[Levye] Configured Android signing alias is invalid.\n";
      return false;
    }
    auto keystore = fs::path(project->androidSigningKeystore);

    if (keystore.is_relative()) {
      keystore = projectDirectory / keystore;
    }

    if (!fs::is_regular_file(keystore)) {
      std::cerr << "[Levye] Configured keystore is missing: " << keystore << '\n';
      return false;
    }

    return true;
  }

  if (hasKeystore || hasAlias) {
    std::cerr << "[Levye] Incomplete signing configuration.\n";
    std::cerr << "[Levye] Both keystore and alias are required.\n";
    return false;
  }

  if (!IsInteractive()) {
    std::cerr << "[Levye] Android production signing is not configured.\n"
              << "[Levye] Run export interactively or configure "
                 "[android.signing].\n";

    return false;
  }

  std::cout << "\n[Levye] Android production signing setup\n\n"
            << "  1. Generate a new keystore\n"
            << "  2. Use an existing keystore\n"
            << "  3. Cancel\n\n";

  const auto choice = Prompt("Select");

  if (!choice || *choice == "3") {
    return false;
  }

  fs::path keystore;

  if (*choice == "1") {
    if (!IsValidPackage(project->androidPackage)) {
      std::cerr << "[Levye] A valid Android package is required.\n";
      return false;
    }

    const auto directory = GetSigningDirectory();

    if (directory.empty() || !directory.is_absolute()) {
      std::cerr << "[Levye] Cannot locate user signing directory.\n";
      return false;
    }

    std::error_code pathError;
    const auto projectRoot = fs::weakly_canonical(projectDirectory, pathError);
    if (pathError) return false;
    const auto signingRoot = fs::weakly_canonical(directory, pathError);
    if (pathError) return false;
    const auto relative = signingRoot.lexically_relative(projectRoot);
    if (!relative.empty() && !relative.is_absolute() && *relative.begin() != "..") {
      std::cerr << "[Levye] Production keys must be generated outside the project.\n";
      return false;
    }

    keystore = directory / project->androidPackage / "release.p12";

    if (fs::exists(keystore)) {
      std::cerr << "[Levye] A keystore already exists at:\n"
                << keystore << '\n'
                << "[Levye] Select option 2 to reuse it.\n";

      return false;
    }

  } else if (*choice == "2") {
    const auto path = Prompt("Keystore path");

    if (!path || path->empty()) {
      return false;
    }

    keystore = fs::path(*path);

    if (keystore.is_relative()) {
      keystore = projectDirectory / keystore;
    }

    if (!fs::is_regular_file(keystore)) {
      std::cerr << "[Levye] Keystore not found.\n";
      return false;
    }

  } else {
    std::cerr << "[Levye] Invalid selection.\n";
    return false;
  }

  const auto alias = Prompt("Key alias", "release");

  if (!alias || !IsValidAlias(*alias)) {
    std::cerr << "[Levye] Invalid key alias.\n";
    return false;
  }

  if (*choice == "1") {
    std::cout << "[Levye] Generating production keystore...\n";

    if (!GenerateKeystore(keystore, *alias)) {
      return false;
    }
  } else {
    if (!VerifyKeystore(keystore, *alias)) {
      return false;
    }
  }

  std::error_code error;
  keystore = fs::canonical(keystore, error);

  if (error) {
    return false;
  }

  if (!SaveConfiguration(projectDirectory, keystore, *alias)) {
    std::cerr << "[Levye] Your keystore remains at: " << keystore << '\n';

    return false;
  }

  std::cout << "\n[Levye] Production signing configured.\n"
            << "[Levye] Keystore: " << keystore << '\n'
            << "[Levye] Keep a secure backup of this keystore.\n\n";

  return true;
}

}  // namespace Levye
