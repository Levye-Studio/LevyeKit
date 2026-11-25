#include "TouchManager.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace Levye {

void TouchManager::Update(const std::vector<RawTouchPoint>& active, Convert convert,
                          void* context) {
  std::vector<TouchPoint> next;
  next.reserve(active.size() + snapshot_.size());
  std::unordered_set<int> seen;
  for (const auto& raw : active) {
    if (raw.id < 0 || !std::isfinite(raw.x) || !std::isfinite(raw.y) || seen.count(raw.id))
      continue;
    float x = raw.x, y = raw.y;
    bool inside = true;
    if (convert)
      inside = convert(raw.x, raw.y, x, y, context);
    if (!std::isfinite(x) || !std::isfinite(y))
      continue;
    seen.insert(raw.id);
    const TouchPoint* previous = FindTouch(raw.id);
    const bool existed = previous && previous->phase != TouchPhase::Ended &&
                         previous->phase != TouchPhase::Cancelled;
    TouchPoint point;
    point.id = raw.id;
    point.x = x;
    point.y = y;
    point.insideViewport = inside;
    if (existed) {
      point.deltaX = x - previous->x;
      point.deltaY = y - previous->y;
      point.phase =
          point.deltaX != 0 || point.deltaY != 0 ? TouchPhase::Moved : TouchPhase::Stationary;
    } else
      point.phase = TouchPhase::Began;
    next.push_back(point);
  }
  for (const auto& old : snapshot_) {
    if (old.phase == TouchPhase::Ended || old.phase == TouchPhase::Cancelled || seen.count(old.id))
      continue;
    TouchPoint ended = old;
    ended.deltaX = ended.deltaY = 0;
    ended.phase = TouchPhase::Ended;
    next.push_back(ended);
  }
  snapshot_.swap(next);
}

void TouchManager::CancelAll() {
  std::vector<TouchPoint> cancelled;
  for (auto point : snapshot_) {
    if (point.phase == TouchPhase::Ended || point.phase == TouchPhase::Cancelled)
      continue;
    point.phase = TouchPhase::Cancelled;
    point.deltaX = point.deltaY = 0;
    cancelled.push_back(point);
  }
  snapshot_.swap(cancelled);
}

void TouchManager::Sample(const std::vector<RawTouchPoint>& native, bool mouseDown, float mouseX,
                          float mouseY, bool focused, Convert convert, void* context) {
  if (!focused) {
    CancelAll();
    return;
  }
  if (!native.empty() || !mouseDown || !std::isfinite(mouseX) || !std::isfinite(mouseY)) {
    Update(native, convert, context);
    return;
  }
  Update({{MouseTouchId, mouseX, mouseY}}, convert, context);
}

std::size_t TouchManager::GetTouchCount() const {
  return std::count_if(snapshot_.begin(), snapshot_.end(), [](const TouchPoint& p) {
    return p.phase != TouchPhase::Ended && p.phase != TouchPhase::Cancelled;
  });
}

const TouchPoint* TouchManager::GetTouch(std::size_t index) const {
  // Index addresses active touches; release events are available via FindTouch.
  for (const auto& point : snapshot_) {
    if (point.phase == TouchPhase::Ended || point.phase == TouchPhase::Cancelled)
      continue;
    if (index-- == 0)
      return &point;
  }
  return nullptr;
}

const TouchPoint* TouchManager::GetEvent(std::size_t index) const {
  return index < snapshot_.size() ? &snapshot_[index] : nullptr;
}

const TouchPoint* TouchManager::FindTouch(int id) const {
  auto it = std::find_if(snapshot_.begin(), snapshot_.end(),
                         [id](const TouchPoint& point) { return point.id == id; });
  return it == snapshot_.end() ? nullptr : &*it;
}
}  // namespace Levye
