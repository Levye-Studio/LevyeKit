#pragma once

#include <Levye/Core/GameAPI.hpp>
#include <Levye/HotReload/DynamicLibrary.hpp>

#include <string>

namespace Levye {
/**
 * @brief Manages a reloadable Levye game module.
 *
 * GameModule owns the dynamic library containing game-specific code and
 * retrieves the GameAPI exported by that library.
 *
 * The host application communicates with game code through this class
 * instead of interacting with platform dynamic-library functions directly.
 *
 * A valid Levye game module must export:
 *
 * @code
 * extern "C" Levye::GameAPI GetGameAPI();
 * @endcode
 */
class GameModule {
public:
  GameModule() = default;

  /**
   * @brief Unloads the game module if one is currently loaded.
   */
  ~GameModule();

  GameModule(const GameModule &) = delete;
  GameModule &operator=(const GameModule &) = delete;

  /**
   * @brief Loads a game module from disk.
   *
   * The shared library is opened and its GetGameAPI function is
   * resolved. Loading fails if either operation fails.
   *
   * @param path Path to the game shared library.
   * @return true if the module and its API were loaded successfully.
   */
  bool Load(const std::string &path);

  /**
   * @brief Unloads the current game module.
   *
   * All function pointers obtained from the module become invalid after
   * this function returns.
   */
  void Unload();

  /**
   * @brief Returns the API exported by the currently loaded game module.
   *
   * @return Pointer to the GameAPI, or nullptr if no valid module is
   * currently loaded.
   *
   * @warning The returned API contains function pointers into the loaded
   * dynamic library and must not be used after Unload().
   */
  const GameAPI *GetAPI() const;

  /**
   * @brief Returns whether a valid game module is currently loaded.
   */
  bool IsLoaded() const;

  /**
   * @brief Returns the path of the current game module.
   */
  const std::string &GetPath() const;

private:
  DynamicLibrary m_Library;

  GameAPI m_API{};
  std::string m_Path;

  bool m_HasAPI = false;
};
} // namespace Levye