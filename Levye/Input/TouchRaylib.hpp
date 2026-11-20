#pragma once
#include <raylib.h>

#include <vector>

#include "TouchManager.hpp"

namespace Levye {

// Call after PollInputEvents / raylib's frame event poll, before game callbacks.
inline void SampleRaylibTouches(TouchManager& manager, TouchManager::Convert convert = nullptr,
                                void* context = nullptr) {
  std::vector<RawTouchPoint> touches;
  const int count = GetTouchPointCount();
  touches.reserve(count > 0 ? static_cast<std::size_t>(count) : 0);
  for (int i = 0; i < count; ++i) {
    const int id = GetTouchPointId(i);
    const Vector2 p = GetTouchPosition(i);
    touches.push_back({id, p.x, p.y});
  }
#if defined(PLATFORM_DESKTOP) || defined(__EMSCRIPTEN__)
  const Vector2 mouse = GetMousePosition();
  manager.Sample(touches, IsMouseButtonDown(MOUSE_BUTTON_LEFT), mouse.x, mouse.y, IsWindowFocused(),
                 convert, context);
#else
  manager.Sample(touches, false, 0.0f, 0.0f, IsWindowFocused(), convert, context);
#endif
}
}  // namespace Levye
