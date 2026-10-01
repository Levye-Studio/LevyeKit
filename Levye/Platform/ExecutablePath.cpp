#include "ExecutablePath.hpp"

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#else
#error "ExecutablePath is not implemented for this platform."
#endif

#include <stdexcept>
#include <vector>

namespace Levye {

#if defined(__APPLE__)
std::filesystem::path ExecutablePath::Get() {
  uint32_t size = 0;

  /*
   * Calling _NSGetExecutablePath with a null buffer gives us the
   * required buffer size for the executable path.
   */
  _NSGetExecutablePath(nullptr, &size);

  std::vector<char> buffer(size);

  if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
    throw std::runtime_error("Failed to determine executable path.");
  }

  /*
   * weakly_canonical() resolves relative components and symbolic links
   * where possible without requiring every path component to exist.
   */
  return std::filesystem::weakly_canonical(
      std::filesystem::path(buffer.data()));
}

#elif defined(__linux__)

std::filesystem::path ExecutablePath::Get() {
  std::error_code error;

  /*
   * Linux exposes the running executable through /proc/self/exe.
   * canonical() resolves the symbolic link to the actual executable path.
   */
  const std::filesystem::path path =
      std::filesystem::canonical("/proc/self/exe", error);

  if (error)
    return {};

  return path;
}

#endif

std::filesystem::path ExecutablePath::GetDirectory() {
  return Get().parent_path();
}
} // namespace Levye