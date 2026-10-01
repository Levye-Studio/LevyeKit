#include "DynamicLibrary.hpp"
#include <Levye/Debug/Logger.hpp>

#if defined(__APPLE__) || defined(__linux__)
#include <dlfcn.h>
#else
#error "DynamicLibrary is not implemented for this platform."
#endif


namespace Levye {
DynamicLibrary::~DynamicLibrary() { Unload(); }

DynamicLibrary::DynamicLibrary(DynamicLibrary &&other) noexcept
    : m_Handle(other.m_Handle) {
  other.m_Handle = nullptr;
}

DynamicLibrary &DynamicLibrary::operator=(DynamicLibrary &&other) noexcept {
  if (this == &other)
    return *this;

  /*
   * Release any library currently owned by this object before taking
   * ownership of the incoming native handle.
   */
  Unload();

  m_Handle = other.m_Handle;

  /*
   * Ownership has moved. Clearing the source prevents its destructor from
   * unloading the library now owned by this object.
   */
  other.m_Handle = nullptr;

  return *this;
}

bool DynamicLibrary::Load(const std::string &path) {
  // A DynamicLibrary owns only one native library handle at a time.
  Unload();

#if defined(__APPLE__) || defined(__linux__)

  m_Handle = dlopen(path.c_str(), RTLD_NOW);

  if (!m_Handle) {
    const char *error = dlerror();

    Logger::Error("Failed to load dynamic library: " + path);

    if (error) {
      Logger::Error(error);
    }

    return false;
  }

  return true;

#else

  Logger::Error("Dynamic libraries are not implemented for this platform yet.");

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
    Logger::Error("Failed to find symbol '" + name + "': " + error);

    return nullptr;
  }

  return symbol;

#else

  return nullptr;

#endif
}

bool DynamicLibrary::IsLoaded() const { return m_Handle != nullptr; }
} // namespace Levye