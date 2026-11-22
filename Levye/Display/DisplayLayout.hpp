#pragma once
#include "DisplayTypes.hpp"

namespace Levye {
/** @brief Pure window, framebuffer, and logical viewport geometry. */
class DisplayLayout {
 public:
  /** @brief Selects logical resolution and scaling policy. */
  void Configure(DisplayConfig config);

  /** @brief Sets window and framebuffer sizes; both may differ on high-DPI displays. */
  void Resize(int windowWidth, int windowHeight, int framebufferWidth, int framebufferHeight);

  /** @brief Sets both sizes equally for non-high-DPI callers. */
  void Resize(int framebufferWidth, int framebufferHeight);

  /** @brief Sets safe insets measured in framebuffer pixels. */
  void SetSafeInsets(DisplayInsets physicalInsets);

  /** @brief Reports whether all sizes are positive and conversions are usable. */
  [[nodiscard]] bool IsValid() const;

  /** @brief Returns the visible logical canvas size (expanded on wider/taller drawables). */
  [[nodiscard]] DisplaySize GetLogicalSize() const;
  /** @brief Returns the window size used by raylib drawing and pointer input. */
  [[nodiscard]] DisplaySize GetWindowSize() const { return window_; }
  /** @brief Returns the physical framebuffer size. */
  [[nodiscard]] DisplaySize GetPhysicalSize() const { return framebuffer_; }
  /** @brief Returns the active scaling policy. */
  [[nodiscard]] DisplayScaling GetScaling() const { return config_.scaling; }
  /** @brief Returns the current window orientation. */
  [[nodiscard]] DisplayOrientation GetOrientation() const;
  /** @brief Returns the gameplay viewport in framebuffer pixels. */
  [[nodiscard]] DisplayRect GetViewport() const;
  /** @brief Returns the presentation viewport in raylib window coordinates. */
  [[nodiscard]] DisplayRect GetWindowViewport() const;
  /** @brief Returns presentation pixels per logical unit on each axis. */
  [[nodiscard]] DisplayPoint GetScale() const;
  /** @brief Returns logical coordinates currently visible in the drawable. */
  [[nodiscard]] DisplayRect GetVisibleLogicalArea() const;
  /** @brief Returns the safe rectangle in logical coordinates, clipped to the game. */
  [[nodiscard]] DisplayRect GetSafeArea() const;
  /**
   * @brief Converts a raylib window position into logical game coordinates.
   * @param windowPosition Position from raylib mouse or touch input.
   * @return Logical position and whether it is inside the visible framebuffer viewport.
   * Invalid dimensions or nonfinite input return a zero point marked outside.
   */
  [[nodiscard]] DisplayPosition ScreenToLogical(DisplayPoint windowPosition) const;
  /**
   * @brief Converts a logical game position into raylib window coordinates.
   * @param logical Position in the render target.
   * @return Window position, or zero for invalid dimensions or nonfinite input.
   */
  [[nodiscard]] DisplayPoint LogicalToScreen(DisplayPoint logical) const;
  /** @brief Converts a raylib window position into framebuffer pixels. */
  [[nodiscard]] DisplayPoint WindowToFramebuffer(DisplayPoint windowPosition) const;
  /** @brief Converts framebuffer pixels into raylib window coordinates. */
  [[nodiscard]] DisplayPoint FramebufferToWindow(DisplayPoint physicalPosition) const;

 private:
  DisplayConfig config_{};
  DisplaySize window_{};
  DisplaySize framebuffer_{};
  DisplayInsets insets_{};
};
}  // namespace Levye
