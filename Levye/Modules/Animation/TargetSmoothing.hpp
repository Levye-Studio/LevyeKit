#pragma once

#include <raylib.h>

#include <cmath>

namespace Levye {

/** @brief Exponentially approaches a target without overshoot; invalid parameters preserve current.
 */
[[nodiscard]] inline Vector3 SmoothTarget(Vector3 current, Vector3 desired, float deltaTime,
                                          float responseRate) noexcept {
  const auto finite = [](Vector3 value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
  };
  if (!finite(current)) return finite(desired) ? desired : Vector3{};
  if (!finite(desired) || !std::isfinite(deltaTime) || deltaTime < 0.0f ||
      !std::isfinite(responseRate) || responseRate < 0.0f)
    return current;
  if (deltaTime == 0.0f || responseRate == 0.0f) return current;
  const float alpha = 1.0f - std::exp(-responseRate * deltaTime);
  const Vector3 result{static_cast<float>(static_cast<double>(current.x) +
                                          (static_cast<double>(desired.x) - current.x) * alpha),
                       static_cast<float>(static_cast<double>(current.y) +
                                          (static_cast<double>(desired.y) - current.y) * alpha),
                       static_cast<float>(static_cast<double>(current.z) +
                                          (static_cast<double>(desired.z) - current.z) * alpha)};
  return finite(result) ? result : desired;
}

}  // namespace Levye
