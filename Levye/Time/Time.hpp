#pragma once

#include <Levye/Core/Services.hpp>

namespace Levye {

/**
 * @brief Provides access to the host-owned game clock.
 *
 * Time exposes frame timing, accumulated time, time scaling, pause state,
 * and fixed-step interpolation to reloadable game code through HostServices.
 */
class Time {
public:
  /**
   * @brief Returns the scaled time elapsed since the previous frame.
   *
   * @return Scaled frame delta time in seconds.
   */
  static float Delta() {
    const HostServices *host = Services::Get();

    if (!host || !host->GetDeltaTime)
      return 0.0f;

    return host->GetDeltaTime(host->context);
  }

  /**
   * @brief Returns the unscaled time elapsed since the previous frame.
   *
   * This value is unaffected by time scaling or pause state.
   *
   * @return Unscaled frame delta time in seconds.
   */
  static float UnscaledDelta() {
    const HostServices *host = Services::Get();

    if (!host || !host->GetUnscaledDeltaTime)
      return 0.0f;

    return host->GetUnscaledDeltaTime(host->context);
  }

  /**
   * @brief Returns accumulated scaled game time.
   *
   * @return Scaled game time in seconds.
   */
  static double Now() {
    const HostServices *host = Services::Get();

    if (!host || !host->GetTime)
      return 0.0;

    return host->GetTime(host->context);
  }

  /**
   * @brief Returns accumulated unscaled game time.
   *
   * @return Unscaled game time in seconds.
   */
  static double UnscaledNow() {
    const HostServices *host = Services::Get();

    if (!host || !host->GetUnscaledTime)
      return 0.0;

    return host->GetUnscaledTime(host->context);
  }

  /**
   * @brief Changes the scale applied to game time.
   *
   * @param scale New time scale.
   */
  static void SetScale(float scale) {
    const HostServices *host = Services::Get();

    if (!host || !host->SetTimeScale)
      return;

    host->SetTimeScale(host->context, scale);
  }

  /**
   * @brief Returns the current game-time scale.
   *
   * @return Current time scale, or 1 when host services are unavailable.
   */
  static float Scale() {
    const HostServices *host = Services::Get();

    if (!host || !host->GetTimeScale)
      return 1.0f;

    return host->GetTimeScale(host->context);
  }

  /**
   * @brief Changes whether scaled game time is paused.
   *
   * @param paused true to pause scaled game time.
   */
  static void SetPaused(bool paused) {
    const HostServices *host = Services::Get();

    if (!host || !host->SetPaused)
      return;

    host->SetPaused(host->context, paused);
  }

  /**
   * @brief Returns whether scaled game time is paused.
   *
   * @return true when game time is paused.
   */
  static bool IsPaused() {
    const HostServices *host = Services::Get();

    if (!host || !host->IsPaused)
      return false;

    return host->IsPaused(host->context);
  }

  /**
   * @brief Returns the interpolation factor for fixed-step rendering.
   *
   * The value represents how far the current rendered frame lies between
   * the previous and next fixed simulation states.
   *
   * @return Fixed-step interpolation value.
   */
  static float InterpolationAlpha() {
    const HostServices *host = Services::Get();

    if (!host || !host->GetInterpolationAlpha)
      return 0.0f;

    return host->GetInterpolationAlpha(host->context);
  }
};

} // namespace Levye