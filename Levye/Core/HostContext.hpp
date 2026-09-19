#pragma once

namespace Levye {
class InputMap;
class ScreenManager;
class TextureManager;
class AudioManager;

/**
 * @brief Internal collection of host-owned systems exposed to game services.
 *
 * Game modules receive this structure only as an opaque void pointer
 * through HostServices. They should never access HostContext directly.
 */
struct HostContext {
  InputMap *input = nullptr;
  ScreenManager *screens = nullptr;
  TextureManager *textures = nullptr;
  AudioManager *audio = nullptr;
};
} // namespace Levye