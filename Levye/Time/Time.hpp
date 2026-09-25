#pragma once

namespace Levye {
/**
 * @brief Stores timing information for the running game.
 *
 * Time is owned by the host so timing state remains available while the
 * reloadable game module is replaced.
 *
 * The system keeps both scaled and unscaled time values. Scaled time is
 * affected by pause and time scale, while unscaled time always represents
 * the actual frame duration reported by the host.
 */
class Time {
public:
  Time() = default;

  /**
   * @brief Advances the time system by one frame.
   *
   * This should be called exactly once per application frame.
   *
   * @param deltaTime Unscaled frame duration in seconds.
   */
  void Update(float deltaTime);

  /**
   * @brief Returns the scaled duration of the current frame.
   *
   * Returns zero while paused.
   */
  float GetDeltaTime() const;

  /**
   * @brief Returns the real duration of the current frame.
   *
   * This value is not affected by pause or time scale.
   */
  float GetUnscaledDeltaTime() const;

  /**
   * @brief Returns total scaled runtime in seconds.
   */
  double GetTime() const;

  /**
   * @brief Returns total unscaled runtime in seconds.
   */
  double GetUnscaledTime() const;

  /**
   * @brief Changes the speed at which scaled game time advances.
   *
   * A scale of 1.0 represents normal speed. A scale of 0.5 represents
   * half speed and 2.0 represents double speed.
   *
   * Negative values are clamped to zero.
   *
   * @param scale New time scale.
   */
  void SetTimeScale(float scale);

  /**
   * @brief Returns the current game time scale.
   */
  float GetTimeScale() const;

  /**
   * @brief Pauses or resumes scaled game time.
   *
   * Pausing does not affect unscaled timing.
   *
   * @param paused true to pause scaled game time.
   */
  void SetPaused(bool paused);

  /**
   * @brief Returns whether scaled game time is paused.
   */
  bool IsPaused() const;

private:
  float m_DeltaTime = 0.0f;
  float m_UnscaledDeltaTime = 0.0f;

  double m_Time = 0.0;
  double m_UnscaledTime = 0.0;

  float m_TimeScale = 1.0f;

  bool m_Paused = false;
};
} // namespace Levye