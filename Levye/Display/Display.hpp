#pragma once

#include <Levye/Core/Services.hpp>
#include <Levye/Display/DisplayTypes.hpp>

namespace Levye {
/** @brief Game-facing access to the host-owned logical display. */
class Display {
 public:
  /**
   * @brief Copies display sizes and viewports into a plain value.
   * @param info Output value. Its enabled flag is false when logical rendering is off.
   * @return true when host services are bound and display information is available.
   */
  [[nodiscard]] static bool GetInfo(DisplayInfo& info) {
    const HostServices* host = Services::Get();
    return host && host->GetDisplayInfo && host->GetDisplayInfo(host->context, &info);
  }

  /** @brief Reports whether the host renders into a logical resolution. */
  [[nodiscard]] static bool IsEnabled() {
    DisplayInfo info;
    return GetInfo(info) && info.enabled;
  }

  /** @brief Returns the logical game resolution, or zero when unavailable. */
  [[nodiscard]] static DisplaySize GetLogicalSize() {
    DisplayInfo info;
    return GetInfo(info) ? info.logical : DisplaySize{};
  }

  /** @brief Returns the raylib window size used by mouse and touch input. */
  [[nodiscard]] static DisplaySize GetWindowSize() {
    DisplayInfo info;
    return GetInfo(info) ? info.window : DisplaySize{};
  }

  /** @brief Returns the framebuffer size in physical pixels. */
  [[nodiscard]] static DisplaySize GetFramebufferSize() {
    DisplayInfo info;
    return GetInfo(info) ? info.framebuffer : DisplaySize{};
  }

  /** @brief Returns the game viewport in framebuffer pixels. */
  [[nodiscard]] static DisplayRect GetViewport() {
    DisplayInfo info;
    return GetInfo(info) ? info.viewport : DisplayRect{};
  }

  /** @brief Returns the game viewport in raylib window coordinates. */
  [[nodiscard]] static DisplayRect GetWindowViewport() {
    DisplayInfo info;
    return GetInfo(info) ? info.windowViewport : DisplayRect{};
  }

  /** @brief Returns the visible safe rectangle in logical game coordinates. */
  [[nodiscard]] static DisplayRect GetSafeArea() {
    DisplayInfo info;
    return GetInfo(info) ? info.safeArea : DisplayRect{};
  }

  /** @brief Returns the current window orientation. */
  [[nodiscard]] static DisplayOrientation GetOrientation() {
    DisplayInfo info;
    return GetInfo(info) ? info.orientation : DisplayOrientation::Square;
  }

  /** @brief Returns the active scaling policy. */
  [[nodiscard]] static DisplayScaling GetScaling() {
    DisplayInfo info;
    return GetInfo(info) ? info.scaling : DisplayScaling::Fit;
  }

  /**
   * @brief Converts a raylib window position to logical game coordinates.
   * @param position Position from GetMousePosition or GetTouchPosition before host conversion.
   * @return Logical position and whether it lies in the visible game viewport.
   * When logical rendering is disabled, the position is unchanged.
   */
  [[nodiscard]] static DisplayPosition ScreenToLogical(DisplayPoint position) {
    const HostServices* host = Services::Get();
    return host && host->DisplayScreenToLogical
               ? host->DisplayScreenToLogical(host->context, position)
               : DisplayPosition{};
  }

  /**
   * @brief Converts a logical game position to raylib window coordinates.
   * @param position Logical game position.
   * @return Window position, or zero when services are unavailable.
   * When logical rendering is disabled, the position is unchanged.
   */
  [[nodiscard]] static DisplayPoint LogicalToScreen(DisplayPoint position) {
    const HostServices* host = Services::Get();
    return host && host->DisplayLogicalToScreen
               ? host->DisplayLogicalToScreen(host->context, position)
               : DisplayPoint{};
  }

  /**
   * @brief Changes Fit, Fill, or Stretch at the next frame without reallocating.
   * @return false when logical rendering is disabled or the mode is invalid.
   */
  [[nodiscard]] static bool SetScaling(DisplayScaling scaling) {
    const HostServices* host = Services::Get();
    return host && host->SetDisplayScaling && host->SetDisplayScaling(host->context, scaling);
  }
};
}  // namespace Levye
