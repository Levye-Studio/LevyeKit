#pragma once

namespace Levye {

/** @brief Phase of one host-frame touch snapshot. */
enum class TouchPhase { Began, Moved, Stationary, Ended, Cancelled };

/** @brief One touch in logical coordinates when Display is enabled, raw otherwise. */
struct TouchPoint {
  /** @brief Stable platform ID; the desktop mouse uses a reserved positive ID. */
  int id = -1;
  float x = 0, y = 0;
  float deltaX = 0, deltaY = 0;
  TouchPhase phase = TouchPhase::Stationary;
  /** @brief True when the touch lies in the visible game viewport. */
  bool insideViewport = false;
};

/** @brief Raylib window-coordinate input passed to the host-owned touch sampler. */
struct RawTouchPoint {
  int id;
  float x, y;
};
}  // namespace Levye
