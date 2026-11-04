#pragma once
#include <Levye/Assets/AssetHandle.hpp>
namespace Sandbox {
/** @brief Host-owned Sandbox sound and music handles with playback controls. */
struct AudioScene {
  Levye::AssetHandle clickSound{};
  Levye::AssetHandle music{};
  void Initialize();
  void Update();
};
}  // namespace Sandbox
