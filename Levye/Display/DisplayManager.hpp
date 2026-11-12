
#pragma once

#include <raylib.h>

#include <optional>

#include "DisplayLayout.hpp"

namespace Levye {

/** @brief Host-owned logical render target and presentation viewport. */
class DisplayManager {
 public:
  DisplayManager() = default;
  DisplayManager(const DisplayManager&) = delete;
  DisplayManager& operator=(const DisplayManager&) = delete;
  DisplayManager(DisplayManager&&) = delete;
  DisplayManager& operator=(DisplayManager&&) = delete;
  /**
   * @brief Allocates one logical render texture after the window is ready.
   * @return false for invalid configuration or texture allocation failure.
   * The host owns the target until Shutdown(), before the window closes.
   */
  [[nodiscard]] bool Initialize(DisplayConfig config);
  /** @brief Releases the render texture before the graphics context closes. */
  void Shutdown();

  /** @brief Updates layout dimensions without recreating the render texture. */
  void Resize(int windowWidth, int windowHeight, int framebufferWidth, int framebufferHeight);
  /** @brief Schedules a scaling change for the next frame without reallocating. */
  void SetScaling(DisplayScaling scaling);
  /** @brief Sets safe insets in framebuffer pixels. */
  void SetSafeInsets(DisplayInsets insets);

  /** @brief Begins drawing game content into the logical render target. */
  void BeginGame();
  /** @brief Ends logical game rendering. */
  void EndGame();
  /** @brief Presents the logical target into the current window viewport. */
  void Present();

  /** @brief Reports whether the render texture is valid. */
  [[nodiscard]] bool IsReady() const { return ready_; }
  /** @brief Returns host-owned layout state for host-side use only. */
  [[nodiscard]] const DisplayLayout& GetLayout() const { return layout_; }

  /**
   * @brief Converts a raylib window-coordinate point to logical game space.
   * @param p Position returned by raylib mouse or touch input.
   * @return Converted point and visible-viewport membership.
   */
  [[nodiscard]] DisplayPosition ScreenToLogical(DisplayPoint p) const {
    return layout_.ScreenToLogical(p);
  }

  /**
   * @brief Converts a logical game point to raylib window coordinates.
   * @param p Position in the logical render target.
   * @return Window coordinate; may be outside the window in Fill mode.
   */
  [[nodiscard]] DisplayPoint LogicalToScreen(DisplayPoint p) const {
    return layout_.LogicalToScreen(p);
  }

 private:
  DisplayLayout layout_;
  RenderTexture2D target_{};
  bool ready_ = false;
  bool drawing_ = false;
  std::optional<DisplayScaling> pendingScaling_;
};

}  // namespace Levye
