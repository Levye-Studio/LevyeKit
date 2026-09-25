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

  /**
   * @brief Returns the duration of one fixed simulation step.
   *
   * The value is affected by the current time scale but remains constant
   * between fixed updates.
   */
  float GetFixedDeltaTime() const;

  /**
   * @brief Returns the unscaled duration of one fixed simulation step.
   */
  float GetUnscaledFixedDeltaTime() const;

  /**
   * @brief Changes the frequency of fixed simulation updates.
   *
   * @param updatesPerSecond Number of fixed updates performed per second.
   *
   * Values less than or equal to zero are ignored.
   */
  void SetFixedUpdateRate(float updatesPerSecond);

  /**
   * @brief Returns the configured fixed-update frequency.
   */
  float GetFixedUpdateRate() const;

  /**
   * @brief Adds the current scaled frame duration to the fixed-step
   * accumulator.
   *
   * @return Number of fixed simulation steps that should run this frame.
   */
  int ConsumeFixedSteps();

  /**
   * @brief Returns interpolation progress between the previous and next
   * fixed simulation state.
   *
   * The returned value is normally in the range [0, 1).
   */
  float GetInterpolationAlpha() const;

private:
  float m_DeltaTime = 0.0f;
  float m_UnscaledDeltaTime = 0.0f;

  double m_Time = 0.0;
  double m_UnscaledTime = 0.0;

  float m_TimeScale = 1.0f;

  bool m_Paused = false;

  double m_FixedAccumulator = 0.0;

  float m_FixedUpdateRate = 60.0f;
  float m_UnscaledFixedDeltaTime = 1.0f / 60.0f;

  static constexpr int MAX_FIXED_STEPS_PER_FRAME = 8;
};
} // namespace Levye