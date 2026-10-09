#pragma once

#include <string>

namespace Levye {
class InputMap;
class ScreenManager;
class TextureManager;
class AudioManager;
class ShaderManager;
class TimeSystem;
class FontManager;
class SerializationService;
struct SerializationAPI;
struct ImGuiAPI;
class Application;

/**
 * @brief Internal collection of host-owned systems exposed to game services.
 *
 * Game modules receive this structure only as an opaque void pointer
 * through HostServices. They should never access HostContext directly.
 */
struct HostContext {
  InputMap* input = nullptr;
  ScreenManager* screens = nullptr;
  TextureManager* textures = nullptr;
  AudioManager* audio = nullptr;
  ShaderManager* shaders = nullptr;
  FontManager* fonts = nullptr;
  SerializationService* serialization = nullptr;
  SerializationAPI* serializationAPI = nullptr;
  ImGuiAPI* imguiAPI = nullptr;

  TimeSystem* time = nullptr;

  /**
   * @brief Root directory used to resolve relative game asset paths.
   */
  const std::string* assetRoot = nullptr;

  /**
   * @brief Running application that owns the host loop.
   */
  Application* application = nullptr;
};
}  // namespace Levye