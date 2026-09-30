#pragma once

#include <string>

namespace Levye {
/**
 * @brief Platform-independent wrapper around a native dynamic library.
 *
 * DynamicLibrary is responsible for loading shared libraries and resolving
 * exported symbols.
 *
 * Platform-specific operations such as dlopen() on macOS/Linux or
 * LoadLibrary() on Windows are hidden behind this class.
 */
class DynamicLibrary {
public:
  DynamicLibrary() = default;

  /**
   * @brief Ensures the currently loaded library is released.
   */
  ~DynamicLibrary();

  DynamicLibrary(const DynamicLibrary &) = delete;
  DynamicLibrary &operator=(const DynamicLibrary &) = delete;
  /**
   * @brief Transfers ownership of a loaded dynamic library.
   *
   * The source object is left empty and will no longer unload the transferred
   * native library handle.
   *
   * @param other DynamicLibrary whose native handle should be transferred.
   */
  DynamicLibrary(DynamicLibrary &&other) noexcept;

  /**
   * @brief Replaces this library with another DynamicLibrary's native handle.
   *
   * Any library currently owned by this object is unloaded before ownership is
   * transferred. The source object is left empty.
   *
   * @param other DynamicLibrary whose native handle should be transferred.
   * @return Reference to this DynamicLibrary.
   */
  DynamicLibrary &operator=(DynamicLibrary &&other) noexcept;

  /**
   * @brief Loads a dynamic library from disk.
   *
   * @param path Path to the dynamic library.
   * @return true when the library was loaded successfully.
   */
  bool Load(const std::string &path);

  /**
   * @brief Unloads the currently loaded dynamic library.
   *
   * Calling this function when no library is loaded is safe.
   */
  void Unload();

  /**
   * @brief Finds an exported symbol inside the loaded library.
   *
   * @param name Name of the exported function or variable.
   * @return Pointer to the symbol, or nullptr when it cannot be found.
   *
   * @warning Returned pointers become invalid after Unload().
   */
  void *GetSymbol(const std::string &name) const;

  /**
   * @brief Returns whether a dynamic library is currently loaded.
   */
  bool IsLoaded() const;

private:
  void *m_Handle = nullptr;
};
} // namespace Levye