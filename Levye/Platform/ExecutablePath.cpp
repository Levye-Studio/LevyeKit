#include "ExecutablePath.hpp"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
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

#elif defined(_WIN32)
std::filesystem::path ExecutablePath::Get() {
  /*
   * GetModuleFileNameW retrieves the absolute path of the
   * executable associated with the current process.
   *
   * Passing nullptr requests the current executable rather
   * than another loaded module.
   */
  std::wstring buffer(260, L'\0');

  while (true) {
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
                                            static_cast<DWORD>(buffer.size()));

    if (length == 0) {
      return {};
    }

    /*
     * When the returned length fits within the buffer,
     * the complete executable path has been retrieved.
     */
    if (length < buffer.size()) {
      buffer.resize(length);

      return std::filesystem::path(buffer);
    }

    /*
     * Windows may truncate long executable paths.
     * Increase the buffer and try again.
     */
    if (buffer.size() >= 32768) {
      return {};
    }

    buffer.resize(buffer.size() * 2);
  }
}

#endif

std::filesystem::path ExecutablePath::GetDirectory() {
  return Get().parent_path();
}
} // namespace Levye