#pragma once
#include <cstddef>
#include <limits>
#include <vector>

#include "TouchTypes.hpp"

namespace Levye {

/** @brief Host-owned touch snapshots updated once per frame without consuming reads. */
class TouchManager {
 public:
  /** @brief Identifier reserved for a synthetic mouse touch. */
  static constexpr int MouseTouchId = std::numeric_limits<int>::max();
  /** @brief Converts window coordinates to logical coordinates for a touch sample. */
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
  /** @brief Returns the number of currently active touches. */
  [[nodiscard]] std::size_t GetTouchCount() const;
  /** @brief Returns the number of touch changes retained for this frame. */
  [[nodiscard]] std::size_t GetEventCount() const {
    return snapshot_.size();
  }
  /** @brief Returns an active touch by index, or null when out of range. */
  [[nodiscard]] const TouchPoint* GetTouch(std::size_t index) const;
  /** @brief Returns a frame touch event by index, or null when out of range. */
  [[nodiscard]] const TouchPoint* GetEvent(std::size_t index) const;
  /** @brief Finds an active touch by its identifier. */
  [[nodiscard]] const TouchPoint* FindTouch(int id) const;

 private:
  std::vector<TouchPoint> snapshot_;
};
}  // namespace Levye
