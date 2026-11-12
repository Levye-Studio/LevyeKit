#pragma once

#include <type_traits>

namespace Levye {
/** @brief How a logical game frame fits the window. */
enum class DisplayScaling { Fit, Fill, Stretch };
/** @brief Orientation of the current window dimensions. */
enum class DisplayOrientation { Portrait, Landscape, Square };
/** @brief Integral width and height in the coordinate space documented by the caller. */
struct DisplaySize {
  int width = 0;
  int height = 0;
};
/** @brief Floating-point X and Y coordinates. */
struct DisplayPoint {
  float x = 0.0f;
  float y = 0.0f;
};
/** @brief Floating-point rectangle with a top-left origin. */
struct DisplayRect {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
};
/** @brief Unusable edges measured in framebuffer pixels. */
struct DisplayInsets {
  float left = 0, top = 0, right = 0, bottom = 0;
};
/** @brief Logical render size and viewport scaling policy. */
struct DisplayConfig {
  DisplaySize logical{1280, 720};
  DisplayScaling scaling = DisplayScaling::Fit;
};
/** @brief Converted point and its visible gameplay-viewport membership. */
struct DisplayPosition {
  DisplayPoint point{};
  bool insideViewport = false;
};
/** @brief Value snapshot of host display state; no host pointers cross the module ABI. */
struct DisplayInfo {
  /** @brief Whether a logical render target is active. */
  bool enabled = false;
  /** @brief Logical game resolution; equals window size when disabled. */
  DisplaySize logical{};
  /** @brief Raylib window/input dimensions. */
  DisplaySize window{};
  /** @brief Render framebuffer dimensions in physical pixels. */
  DisplaySize framebuffer{};
  /** @brief Visible game viewport in framebuffer pixels. */
  DisplayRect viewport{};
  /** @brief Presentation rectangle in raylib window coordinates. */
  DisplayRect windowViewport{};
  /** @brief Safe rectangle clipped to the visible game, in logical coordinates. */
  DisplayRect safeArea{};
  /** @brief Orientation of the current window. */
  DisplayOrientation orientation = DisplayOrientation::Square;
  /** @brief Active scaling mode; changes take effect at the next frame boundary. */
  DisplayScaling scaling = DisplayScaling::Fit;
};
static_assert(std::is_trivially_copyable_v<DisplayInfo>);
}  // namespace Levye
