#include "Time.hpp"

#include <algorithm>

namespace Levye {
void Time::Update(float deltaTime) {
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

float Time::GetDeltaTime() const { return m_DeltaTime; }

float Time::GetUnscaledDeltaTime() const { return m_UnscaledDeltaTime; }

double Time::GetTime() const { return m_Time; }

double Time::GetUnscaledTime() const { return m_UnscaledTime; }

void Time::SetTimeScale(float scale) { m_TimeScale = std::max(scale, 0.0f); }

float Time::GetTimeScale() const { return m_TimeScale; }

void Time::SetPaused(bool paused) { m_Paused = paused; }

bool Time::IsPaused() const { return m_Paused; }

float Time::GetFixedDeltaTime() const { return m_UnscaledFixedDeltaTime; }

float Time::GetUnscaledFixedDeltaTime() const {
  return m_UnscaledFixedDeltaTime;
}

void Time::SetFixedUpdateRate(float updatesPerSecond) {
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

float Time::GetFixedUpdateRate() const { return m_FixedUpdateRate; }

int Time::ConsumeFixedSteps() {
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

float Time::GetInterpolationAlpha() const {
  const double fixedStep = static_cast<double>(GetFixedDeltaTime());

  if (fixedStep <= 0.0)
    return 0.0f;

  return static_cast<float>(m_FixedAccumulator / fixedStep);
}

float Time::Interpolate(float previous, float current) const {
  const float alpha = GetInterpolationAlpha();

  return previous + (current - previous) * alpha;
}
} // namespace Levye