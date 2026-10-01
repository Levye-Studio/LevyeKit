#include "BuildCommand.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

/**
 * @brief Wraps a filesystem path in quotes for use in a shell command.
 *
 * @param path Filesystem path to quote.
 * @return Quoted path string.
 */
std::string QuotePath(const std::filesystem::path &path) {
  return "\"" + path.string() + "\"";
}

} // namespace

namespace Levye {

bool BuildCommand::Execute(const std::filesystem::path &projectDirectory,
                           BuildConfiguration configuration) {
  const std::string configurationName{ToString(configuration)};

  const std::filesystem::path buildDirectory =
      projectDirectory / "Build" / configurationName;

  std::cout << "[Levye] Configuring project...\n";

  const std::string configureCommand =
      "cmake -S " + QuotePath(projectDirectory) + " -B " +
      QuotePath(buildDirectory) + " -DCMAKE_BUILD_TYPE=" + configurationName;

  const int configureResult = std::system(configureCommand.c_str());

  if (configureResult != 0) {
    std::cerr << "[Levye] Project configuration failed.\n";

    return false;
  }

  std::cout << "[Levye] Building project...\n";

  const std::string buildCommand = "cmake --build " + QuotePath(buildDirectory);

  const int buildResult = std::system(buildCommand.c_str());

  if (buildResult != 0) {
    std::cerr << "[Levye] Project build failed.\n";

    return false;
  }

  std::cout << "[Levye] Build successful.\n";

  return true;
}

} // namespace Levye