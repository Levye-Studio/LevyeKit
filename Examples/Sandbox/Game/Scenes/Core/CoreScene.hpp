#pragma once
#include <Levye/LevyeKit.hpp>

#include "../Audio/AudioScene.hpp"
namespace Sandbox {
/** @brief Player movement, texture, sound, music, and time controls. */
struct CoreScene {
  Vector2 playerPosition{640.0f, 360.0f};
  float playerSpeed = 250.0f;
  int score = 1;
  Levye::AssetHandle playerTexture{};
  AudioScene audio;
  void Initialize();
  void Update(float deltaTime);
  void Draw() const;
};
}  // namespace Sandbox
