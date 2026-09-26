#include "ExecutablePath.hpp"

#include <mach-o/dyld.h>

#include <stdexcept>
#include <vector>

namespace Levye {
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

std::filesystem::path ExecutablePath::GetDirectory() {
  return Get().parent_path();
}
} // namespace Levye