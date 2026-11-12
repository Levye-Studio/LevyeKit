#pragma once
namespace Sandbox {
/** @brief Registers Sandbox logical input actions with the host. */
struct InputScene {
  static void Bind();
  static void Update();
  /** @brief Draws the host touch snapshot on the Sandbox game screen. */
  static void Draw();
};
}  // namespace Sandbox
