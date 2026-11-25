#include "DisplayLayout.hpp"

#include <algorithm>
#include <cmath>

namespace Levye {

void DisplayLayout::Configure(DisplayConfig config) {
  config_ = config;
}

void DisplayLayout::Resize(int windowWidth, int windowHeight, int framebufferWidth,
                           int framebufferHeight) {
  window_ = {windowWidth, windowHeight};
  framebuffer_ = {framebufferWidth, framebufferHeight};
}

void DisplayLayout::Resize(int width, int height) {
  Resize(width, height, width, height);
}

void DisplayLayout::SetSafeInsets(DisplayInsets insets) {
  insets_ = insets;
}

bool DisplayLayout::IsValid() const {
  const bool scalingValid =
      config_.scaling == DisplayScaling::Fit || config_.scaling == DisplayScaling::Fill ||
      config_.scaling == DisplayScaling::Stretch || config_.scaling == DisplayScaling::Integer ||
      config_.scaling == DisplayScaling::Expand;

  return scalingValid && config_.logical.width > 0 && config_.logical.height > 0 &&
         window_.width > 0 && window_.height > 0 && framebuffer_.width > 0 &&
         framebuffer_.height > 0;
}

DisplaySize DisplayLayout::GetLogicalSize() const {
  if (config_.scaling != DisplayScaling::Expand || framebuffer_.width <= 0 ||
      framebuffer_.height <= 0 || config_.logical.width <= 0 || config_.logical.height <= 0)
    return config_.logical;

  const float scale = std::min(static_cast<float>(framebuffer_.width) / config_.logical.width,
                               static_cast<float>(framebuffer_.height) / config_.logical.height);
  return {
      std::max(config_.logical.width, static_cast<int>(std::ceil(framebuffer_.width / scale))),
      std::max(config_.logical.height, static_cast<int>(std::ceil(framebuffer_.height / scale)))};
}

DisplayOrientation DisplayLayout::GetOrientation() const {
  if (window_.width > window_.height)
    return DisplayOrientation::Landscape;

  if (window_.width < window_.height)
    return DisplayOrientation::Portrait;

  return DisplayOrientation::Square;
}

DisplayRect DisplayLayout::GetViewport() const {
  if (!IsValid())
    return {};

  const float fw = static_cast<float>(framebuffer_.width);
  const float fh = static_cast<float>(framebuffer_.height);

  const auto logical = GetLogicalSize();

  const float lw = static_cast<float>(logical.width);
  const float lh = static_cast<float>(logical.height);

  if (config_.scaling == DisplayScaling::Expand)
    return {0.0f, 0.0f, fw, fh};

  if (config_.scaling == DisplayScaling::Stretch)
    return {0.0f, 0.0f, fw, fh};

  const float sx = fw / lw;
  const float sy = fh / lh;

  float scale = (config_.scaling == DisplayScaling::Fill) ? std::max(sx, sy) : std::min(sx, sy);

  // Small screens use fractional Fit scaling so the full frame remains visible.
  if (config_.scaling == DisplayScaling::Integer && scale >= 1.0f)
    scale = std::floor(scale);

  const float width = lw * scale;
  const float height = lh * scale;

  return {(fw - width) * 0.5f, (fh - height) * 0.5f, width, height};
}

DisplayRect DisplayLayout::GetWindowViewport() const {
  if (!IsValid())
    return {};

  const auto viewport = GetViewport();
  const auto topLeft = FramebufferToWindow({viewport.x, viewport.y});
  const auto bottomRight =
      FramebufferToWindow({viewport.x + viewport.width, viewport.y + viewport.height});

  return {topLeft.x, topLeft.y, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y};
}

DisplayPoint DisplayLayout::GetScale() const {
  if (!IsValid())
    return {};

  const auto viewport = GetViewport();
  const auto logical = GetLogicalSize();

  return {viewport.width / logical.width, viewport.height / logical.height};
}

DisplayRect DisplayLayout::GetVisibleLogicalArea() const {
  if (!IsValid())
    return {};

  const auto a = ScreenToLogical({0.0f, 0.0f}).point;
  const auto b =
      ScreenToLogical({static_cast<float>(window_.width), static_cast<float>(window_.height)})
          .point;
  const auto logical = GetLogicalSize();
  const float x0 = std::clamp(a.x, 0.0f, static_cast<float>(logical.width));
  const float y0 = std::clamp(a.y, 0.0f, static_cast<float>(logical.height));
  const float x1 = std::clamp(b.x, 0.0f, static_cast<float>(logical.width));
  const float y1 = std::clamp(b.y, 0.0f, static_cast<float>(logical.height));

  return {x0, y0, std::max(0.0f, x1 - x0), std::max(0.0f, y1 - y0)};
}

DisplayPoint DisplayLayout::WindowToFramebuffer(DisplayPoint p) const {
  if (!IsValid() || !std::isfinite(p.x) || !std::isfinite(p.y))
    return {};

  const DisplayPoint converted{
      p.x * static_cast<float>(framebuffer_.width) / static_cast<float>(window_.width),
      p.y * static_cast<float>(framebuffer_.height) / static_cast<float>(window_.height)};

  return std::isfinite(converted.x) && std::isfinite(converted.y) ? converted : DisplayPoint{};
}

DisplayPoint DisplayLayout::FramebufferToWindow(DisplayPoint p) const {
  if (!IsValid() || !std::isfinite(p.x) || !std::isfinite(p.y))
    return {};

  const DisplayPoint converted{
      p.x * static_cast<float>(window_.width) / static_cast<float>(framebuffer_.width),
      p.y * static_cast<float>(window_.height) / static_cast<float>(framebuffer_.height)};

  return std::isfinite(converted.x) && std::isfinite(converted.y) ? converted : DisplayPoint{};
}

DisplayPosition DisplayLayout::ScreenToLogical(DisplayPoint windowPosition) const {
  if (!IsValid() || !std::isfinite(windowPosition.x) || !std::isfinite(windowPosition.y))
    return {};

  const auto physical = WindowToFramebuffer(windowPosition);
  const auto viewport = GetViewport();

  DisplayPosition result;

  const auto logical = GetLogicalSize();

  result.point = {(physical.x - viewport.x) * logical.width / viewport.width,
                  (physical.y - viewport.y) * logical.height / viewport.height};
  result.insideViewport =
      windowPosition.x >= 0.0f && windowPosition.y >= 0.0f &&
      windowPosition.x < static_cast<float>(window_.width) &&
      windowPosition.y < static_cast<float>(window_.height) && physical.x >= 0.0f &&
      physical.y >= 0.0f && physical.x < static_cast<float>(framebuffer_.width) &&
      physical.y < static_cast<float>(framebuffer_.height) && physical.x >= viewport.x &&
      physical.y >= viewport.y && physical.x < viewport.x + viewport.width &&
      physical.y < viewport.y + viewport.height;

  return result;
}

DisplayPoint DisplayLayout::LogicalToScreen(DisplayPoint logical) const {
  if (!IsValid() || !std::isfinite(logical.x) || !std::isfinite(logical.y))
    return {};

  const auto viewport = GetViewport();
  const auto size = GetLogicalSize();

  return FramebufferToWindow(
      {viewport.x + logical.x * viewport.width / static_cast<float>(size.width),
       viewport.y + logical.y * viewport.height / static_cast<float>(size.height)});
}

DisplayRect DisplayLayout::GetSafeArea() const {
  if (!IsValid())
    return {};

  const auto logical = GetLogicalSize();

  if (config_.safeArea == DisplaySafeArea::EdgeToEdge)
    return GetVisibleLogicalArea();

  const float fw = static_cast<float>(framebuffer_.width);
  const float fh = static_cast<float>(framebuffer_.height);

  const bool inset = config_.safeArea == DisplaySafeArea::Inset;
  const float left =
      std::clamp(inset && std::isfinite(insets_.left) ? insets_.left : 0.0f, 0.0f, fw);
  const float right =
      std::clamp(inset && std::isfinite(insets_.right) ? insets_.right : 0.0f, 0.0f, fw);
  const float top = std::clamp(inset && std::isfinite(insets_.top) ? insets_.top : 0.0f, 0.0f, fh);
  const float bottom =
      std::clamp(inset && std::isfinite(insets_.bottom) ? insets_.bottom : 0.0f, 0.0f, fh);
  const auto a = ScreenToLogical(FramebufferToWindow({left, top})).point;
  const auto b =
      ScreenToLogical(FramebufferToWindow({std::max(left, fw - right), std::max(top, fh - bottom)}))
          .point;
  const float lw = static_cast<float>(logical.width);
  const float lh = static_cast<float>(logical.height);
  const float x0 = std::clamp(a.x, 0.0f, lw);
  const float x1 = std::clamp(b.x, 0.0f, lw);
  const float y0 = std::clamp(a.y, 0.0f, lh);
  const float y1 = std::clamp(b.y, 0.0f, lh);

  return {x0, y0, std::max(0.0f, x1 - x0), std::max(0.0f, y1 - y0)};
}

}  // namespace Levye
