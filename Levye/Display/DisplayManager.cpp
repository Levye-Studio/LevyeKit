
#include "DisplayManager.hpp"

#include <algorithm>

namespace Levye {

bool DisplayManager::Initialize(DisplayConfig config) {
  Shutdown();

  if (config.logical.width <= 0 || config.logical.height <= 0 ||
      (config.scaling != DisplayScaling::Fit && config.scaling != DisplayScaling::Fill &&
       config.scaling != DisplayScaling::Stretch && config.scaling != DisplayScaling::Integer &&
       config.scaling != DisplayScaling::Expand))
    return false;

  config_ = config;
  layout_.Configure(config);
  layout_.Resize(GetScreenWidth(), GetScreenHeight(), GetRenderWidth(), GetRenderHeight());

  const auto size = layout_.GetLogicalSize();
  target_ = LoadRenderTexture(size.width, size.height);
  if (target_.id != 0)
    SetTextureFilter(target_.texture, config.filter == DisplayFilter::Nearest
                                          ? TEXTURE_FILTER_POINT
                                          : TEXTURE_FILTER_BILINEAR);

  ready_ = target_.id != 0 && target_.texture.id != 0;
  if (!ready_ && target_.id != 0) {
    UnloadRenderTexture(target_);
    target_ = {};
  }
  return ready_;
}

void DisplayManager::Shutdown() {
  if (drawing_) {
    EndTextureMode();
    drawing_ = false;
  }

  if (ready_) {
    UnloadRenderTexture(target_);
  }

  target_ = {};
  ready_ = false;
  pendingScaling_.reset();
}

void DisplayManager::Resize(int windowWidth, int windowHeight, int framebufferWidth,
                            int framebufferHeight) {
  if (pendingScaling_) {
    auto config = config_;
    config.scaling = *pendingScaling_;
    config_ = config;
    layout_.Configure(config);
    pendingScaling_.reset();
  }
  layout_.Resize(windowWidth, windowHeight, framebufferWidth, framebufferHeight);
  const auto size = layout_.GetLogicalSize();
  if (ready_ && !drawing_ &&
      (target_.texture.width != size.width || target_.texture.height != size.height)) {
    auto replacement = LoadRenderTexture(size.width, size.height);
    if (replacement.id != 0 && replacement.texture.id != 0) {
      SetTextureFilter(replacement.texture, config_.filter == DisplayFilter::Nearest
                                                ? TEXTURE_FILTER_POINT
                                                : TEXTURE_FILTER_BILINEAR);
      UnloadRenderTexture(target_);
      target_ = replacement;
    }
  }
}

void DisplayManager::SetScaling(DisplayScaling scaling) {
  if (scaling == DisplayScaling::Fit || scaling == DisplayScaling::Fill ||
      scaling == DisplayScaling::Stretch || scaling == DisplayScaling::Integer ||
      scaling == DisplayScaling::Expand)
    pendingScaling_ = scaling;
}

void DisplayManager::SetSafeInsets(DisplayInsets insets) {
  layout_.SetSafeInsets(insets);
}

void DisplayManager::BeginGame() {
  if (!ready_ || drawing_)
    return;

  BeginTextureMode(target_);
  ClearBackground(BLACK);
  drawing_ = true;
}

void DisplayManager::EndGame() {
  if (!drawing_)
    return;

  EndTextureMode();
  drawing_ = false;
}

void DisplayManager::Present() {
  if (!ready_ || drawing_ || !layout_.IsValid())
    return;

  const DisplayRect viewport = layout_.GetWindowViewport();
  const DisplaySize logical = layout_.GetLogicalSize();

  const Rectangle source{0.0f, 0.0f, static_cast<float>(logical.width),
                         -static_cast<float>(logical.height)};

  const Rectangle destination{viewport.x, viewport.y, viewport.width, viewport.height};

  DrawTexturePro(target_.texture, source, destination, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
}

}  // namespace Levye
