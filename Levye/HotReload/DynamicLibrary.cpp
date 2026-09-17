#include "DynamicLibrary.hpp"

#if defined(__APPLE__) || defined(__linux__)
#include <dlfcn.h>
#endif

#include <iostream>

namespace Levye {
DynamicLibrary::~DynamicLibrary() { Unload(); }

bool DynamicLibrary::Load(const std::string &path) {
  // A DynamicLibrary owns only one native library handle at a time.
  Unload();

#if defined(__APPLE__) || defined(__linux__)

  m_Handle = dlopen(path.c_str(), RTLD_NOW);

  if (!m_Handle) {
    const char *error = dlerror();

    std::cerr << "[LevyeKit] Failed to load dynamic library: " << path << '\n';

    if (error) {
      std::cerr << "[LevyeKit] " << error << '\n';
    }

    return false;
  }

  return true;

#else

  std::cerr << "[LevyeKit] Dynamic libraries are not implemented "
            << "for this platform yet.\n";

  return false;

#endif
}

void DynamicLibrary::Unload() {
  if (!m_Handle)
    return;

#if defined(__APPLE__) || defined(__linux__)

  dlclose(m_Handle);

#endif

  // Symbols obtained from the library are no longer valid after this
  // point, so the handle must not remain accessible.
  m_Handle = nullptr;
}

void *DynamicLibrary::GetSymbol(const std::string &name) const {
  if (!m_Handle)
    return nullptr;

#if defined(__APPLE__) || defined(__linux__)

  // Clear any error left behind by an earlier dynamic-loader operation.
  dlerror();

  void *symbol = dlsym(m_Handle, name.c_str());

  const char *error = dlerror();

  if (error) {
    std::cerr << "[LevyeKit] Failed to find symbol '" << name << "': " << error
              << '\n';

    return nullptr;
  }

  return symbol;

#else

  return nullptr;

#endif
}

bool DynamicLibrary::IsLoaded() const { return m_Handle != nullptr; }
} // namespace Levye