#include "SharedLibrary.hpp"

#include <string>

namespace Levye {

std::filesystem::path SharedLibrary::MakeFilename(std::string_view name) {
  return std::string{name} + std::string{GetExtension()};
}

} // namespace Levye