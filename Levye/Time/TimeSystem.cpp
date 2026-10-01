#include "TimeSystem.hpp"

#include <algorithm>

namespace Levye {
void TimeSystem::Update(float deltaTime) {
  /*
   * Frame duration should never move backward. Protect the rest of the
   * framework if an invalid negative value is supplied.
   */
  m_UnscaledDeltaTime = std::max(deltaTime, 0.0f);

  m_UnscaledTime += static_cast<double>(m_UnscaledDeltaTime);

  if (m_Paused) {
    m_DeltaTime = 0.0f;
    return;
  }

  m_DeltaTime = m_UnscaledDeltaTime * m_TimeScale;

  m_Time += static_cast<double>(m_DeltaTime);
}

float TimeSystem::GetDeltaTime() const { return m_DeltaTime; }

float TimeSystem::GetUnscaledDeltaTime() const { return m_UnscaledDeltaTime; }

double TimeSystem::GetTimeSystem() const { return m_Time; }

double TimeSystem::GetUnscaledTime() const { return m_UnscaledTime; }

void TimeSystem::SetTimeScale(float scale) {
  m_TimeScale = std::max(scale, 0.0f);
}

float TimeSystem::GetTimeScale() const { return m_TimeScale; }

void TimeSystem::SetPaused(bool paused) { m_Paused = paused; }

bool TimeSystem::IsPaused() const { return m_Paused; }

float TimeSystem::GetFixedDeltaTime() const { return m_UnscaledFixedDeltaTime; }

float TimeSystem::GetUnscaledFixedDeltaTime() const {
  return m_UnscaledFixedDeltaTime;
}

void TimeSystem::SetFixedUpdateRate(float updatesPerSecond) {
  if (updatesPerSecond <= 0.0f)
    return;

  m_FixedUpdateRate = updatesPerSecond;

  m_UnscaledFixedDeltaTime = 1.0f / updatesPerSecond;

  /*
   * Reset accumulated time because changing the simulation frequency changes
   * the meaning of any partially accumulated fixed step.
   */
  m_FixedAccumulator = 0.0;
}

float TimeSystem::GetFixedUpdateRate() const { return m_FixedUpdateRate; }

int TimeSystem::ConsumeFixedSteps() {
  if (m_Paused)
    return 0;

  /*
   * Accumulate scaled game time so slow motion and fast-forward affect the
   * rate at which the simulation advances.
   */
  m_FixedAccumulator += static_cast<double>(m_DeltaTime);

  const double fixedStep = static_cast<double>(GetFixedDeltaTime());

  if (fixedStep <= 0.0)
    return 0;

  int steps = 0;

  while (m_FixedAccumulator >= fixedStep && steps < MAX_FIXED_STEPS_PER_FRAME) {
    m_FixedAccumulator -= fixedStep;
    ++steps;
  }

  /*
   * If the application falls severely behind, discard excess accumulated
   * simulation time rather than allowing an unlimited catch-up loop.
   */
  if (steps == MAX_FIXED_STEPS_PER_FRAME && m_FixedAccumulator >= fixedStep) {
    m_FixedAccumulator = 0.0;
  }

  return steps;
}

float TimeSystem::GetInterpolationAlpha() const {
  const double fixedStep = static_cast<double>(GetFixedDeltaTime());

  if (fixedStep <= 0.0)
    return 0.0f;

  return static_cast<float>(m_FixedAccumulator / fixedStep);
}

float TimeSystem::Interpolate(float previous, float current) const {
  const float alpha = GetInterpolationAlpha();

  return previous + (current - previous) * alpha;
}
} // namespace Levye