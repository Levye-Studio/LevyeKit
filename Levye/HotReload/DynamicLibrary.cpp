#include "DynamicLibrary.hpp"
#include <Levye/Debug/Logger.hpp>
#include <filesystem>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__) || defined(__linux__)
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

#if defined(_WIN32)

  /*
   * LoadLibraryW loads the exact runtime copy using an absolute native path.
   *
   * The returned HMODULE represents the loaded DLL and must be
   * released with FreeLibrary when no longer needed.
   */
  const auto nativePath = std::filesystem::absolute(path);
  // A malformed or temporarily incomplete build must not display a loader
  // error dialog on the game thread. Keep this policy local to this call.
  DWORD previousMode = 0;
  const bool changedMode = SetThreadErrorMode(
      SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX, &previousMode) != 0;
  m_Handle = reinterpret_cast<void *>(LoadLibraryW(nativePath.c_str()));
  const DWORD loadError = m_Handle ? ERROR_SUCCESS : GetLastError();
  if (changedMode)
    SetThreadErrorMode(previousMode, nullptr);

  if (!m_Handle) {
    Logger::Error("Failed to load dynamic library: " + path +
                  " (Windows error " + std::to_string(loadError) + ")");

    return false;
  }

#elif defined(__APPLE__) || defined(__linux__)

  /*
   * RTLD_NOW resolves symbols immediately, allowing us to detect
   * unresolved dependencies before activating the game module.
   */
  m_Handle = dlopen(path.c_str(), RTLD_NOW);

  if (!m_Handle) {
    const char *error = dlerror();

    Logger::Error("Failed to load dynamic library: " + path);

    if (error)
      Logger::Error(error);

    return false;
  }

#endif

  return true;
}

void DynamicLibrary::Unload() {
  if (!m_Handle)
    return;

#if defined(_WIN32)

  /*
   * Release the DLL previously loaded with LoadLibraryW.
   */
  if (!FreeLibrary(reinterpret_cast<HMODULE>(m_Handle))) {
    Logger::Error("Failed to unload dynamic library. Windows error: " +
                  std::to_string(GetLastError()));
  }

#elif defined(__APPLE__) || defined(__linux__)

  /*
   * Release the shared library previously loaded with dlopen.
   */
  if (dlclose(m_Handle) != 0) {
    const char *error = dlerror();

    if (error)
      Logger::Error(error);
  }

#endif

  /*
   * Symbols obtained from the library are no longer valid after
   * unloading, so the handle must not remain accessible.
   */
  m_Handle = nullptr;
}

void *DynamicLibrary::GetSymbol(const std::string &name) const {
  if (!m_Handle)
    return nullptr;

#if defined(_WIN32)

  /*
   * GetProcAddress retrieves an exported symbol from the loaded DLL.
   *
   * Game modules expose GetGameAPI with C linkage to avoid C++
   * name mangling.
   */
  FARPROC symbol =
      GetProcAddress(reinterpret_cast<HMODULE>(m_Handle), name.c_str());

  if (!symbol) {
    Logger::Error("Failed to find symbol '" + name +
                  "'. Windows error: " + std::to_string(GetLastError()));

    return nullptr;
  }

  return reinterpret_cast<void *>(symbol);

#elif defined(__APPLE__) || defined(__linux__)

  // Clear any error left behind by an earlier loader operation.
  dlerror();

  void *symbol = dlsym(m_Handle, name.c_str());

  const char *error = dlerror();

  if (error) {
    Logger::Error("Failed to find symbol '" + name + "': " + error);

    return nullptr;
  }

  return symbol;

#endif
}

bool DynamicLibrary::IsLoaded() const { return m_Handle != nullptr; }
} // namespace Levye
