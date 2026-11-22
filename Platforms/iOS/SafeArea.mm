#import <UIKit/UIKit.h>

#include <Levye/Display/DisplayTypes.hpp>

// UIKit reports points. The host converts them to framebuffer pixels.
extern "C" void LevyeIOSSafeInsets(int framebufferWidth, int framebufferHeight,
                                   Levye::DisplayInsets* outputInsets) {
  if (!outputInsets) return;
  *outputInsets = {};
  UIWindow* window = nil;
  for (UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
    if (![scene isKindOfClass:UIWindowScene.class]) continue;
    for (UIWindow* candidate in ((UIWindowScene*)scene).windows) {
      if (candidate.isKeyWindow) {
        window = candidate;
        break;
      }
    }
    if (window) break;
  }
  if (!window || window.bounds.size.width <= 0 || window.bounds.size.height <= 0) return;
  const UIEdgeInsets safe = window.safeAreaInsets;
  const float sx = static_cast<float>(framebufferWidth / window.bounds.size.width);
  const float sy = static_cast<float>(framebufferHeight / window.bounds.size.height);
  *outputInsets = {static_cast<float>(safe.left) * sx, static_cast<float>(safe.top) * sy,
                   static_cast<float>(safe.right) * sx, static_cast<float>(safe.bottom) * sy};
}
