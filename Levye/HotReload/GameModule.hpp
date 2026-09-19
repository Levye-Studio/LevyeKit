#pragma once

#include <Levye/Core/GameAPI.hpp>
#include <Levye/HotReload/DynamicLibrary.hpp>

#include <filesystem>
#include <string>

namespace Levye {
/**
 * @brief Owns and manages a dynamically loaded Levye game module.
 *
 * GameModule owns the module's dynamic library, current GameAPI and
 * persistent GameState.
 *
 * It can detect when the module file changes and replace the loaded game
 * code without destroying the host application or persistent state.
 */
class GameModule {
public:
  GameModule() = default;

  /**
   * @brief Unloads the currently loaded module.
   */
  ~GameModule();

  GameModule(const GameModule &) = delete;
  GameModule &operator=(const GameModule &) = delete;

  /**
   * @brief Loads the initial game module.
   *
   * @param path Path to the game shared library.
   * @return true when the module was loaded successfully.
   */
  bool Load(const std::string &path);

  /**
   * @brief Checks whether the game library changed and reloads it.
   *
   * This function is intended to be called regularly by the host.
   *
   * @return true when a hot reload occurred successfully.
   */
  bool CheckForReload();

  /**
   * @brief Unloads the current game module.
   */
  void Unload();

  /**
   * @brief Calls the current module's update callback.
   *
   * @param deltaTime Time elapsed since the previous frame.
   */
  void Update(float deltaTime);

  /**
   * @brief Calls the current module's drawing callback.
   */
  void Draw();

  /**
   * @brief Returns whether a valid game module is currently loaded.
   */
  bool IsLoaded() const;

  /**
   * @brief Returns the path of the current game module.
   */
  const std::string &GetPath() const;

private:
  /**
   * Loads the dynamic library and resolves its GameAPI without invoking
   * lifecycle callbacks.
   */
  bool LoadLibrary();

  DynamicLibrary m_Library;

  GameAPI m_API{};
  GameState m_State{};

  std::string m_Path;

  std::filesystem::file_time_type m_LastWriteTime{};

  bool m_HasAPI = false;
  bool m_Started = false;
};
} // namespace Levye