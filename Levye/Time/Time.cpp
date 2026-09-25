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
} // namespace Levye