#pragma once
#include <cstddef>
#include <limits>
#include <vector>

#include "TouchTypes.hpp"

namespace Levye {

// Host-owned. Update exactly once per host frame; reads never consume events.
class TouchManager {
 public:
  static constexpr int MouseTouchId = std::numeric_limits<int>::max();
  using Convert = bool (*)(float windowX, float windowY, float& logicalX, float& logicalY,
                           void* context);
  /** @brief Replaces the frame snapshot; returned pointers remain valid until the next update. */
  void Update(const std::vector<RawTouchPoint>& active, Convert convert = nullptr,
              void* context = nullptr);
  /** @brief Exposes cancellation for one frame and removes active touches. */
  void CancelAll();
  /** @brief Samples native touches, or one synthetic mouse touch when none exist. */
  void Sample(const std::vector<RawTouchPoint>& native, bool mouseDown, float mouseX, float mouseY,
              bool focused, Convert convert = nullptr, void* context = nullptr);
  [[nodiscard]] std::size_t GetTouchCount() const;  // active touches only
  [[nodiscard]] std::size_t GetEventCount() const { return snapshot_.size(); }
  [[nodiscard]] const TouchPoint* GetTouch(std::size_t index) const;
  [[nodiscard]] const TouchPoint* GetEvent(std::size_t index) const;
  [[nodiscard]] const TouchPoint* FindTouch(int id) const;

 private:
  std::vector<TouchPoint> snapshot_;
};
}  // namespace Levye
