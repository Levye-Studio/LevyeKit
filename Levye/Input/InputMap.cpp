#include "InputMap.hpp"

#include <algorithm>
#include <cmath>

namespace Levye {
void InputMap::BindKey(const std::string &action, KeyboardKey key) {
  auto &keys = m_Actions[action].keys;

  /*
   * Avoid duplicate bindings because an action only needs to query a
   * physical key once.
   */
  if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
    keys.push_back(key);
  }
}

void InputMap::BindGamepadButton(const std::string &action, int gamepad,
                                 GamepadButton button) {
  auto &buttons = m_Actions[action].gamepadButtons;

  const auto existing = std::find_if(
      buttons.begin(), buttons.end(),
      [gamepad, button](const GamepadButtonBinding &binding) {
        return binding.gamepad == gamepad && binding.button == button;
      });

  if (existing == buttons.end()) {
    buttons.push_back({.gamepad = gamepad, .button = button});
  }
}

void InputMap::BindKeyAxis(const std::string &axis, KeyboardKey negativeKey,
                           KeyboardKey positiveKey) {
  auto &bindings = m_Axes[axis].keyBindings;

  const auto existing =
      std::find_if(bindings.begin(), bindings.end(),
                   [negativeKey, positiveKey](const KeyAxisBinding &binding) {
                     return binding.negativeKey == negativeKey &&
                            binding.positiveKey == positiveKey;
                   });

  if (existing == bindings.end()) {
    bindings.push_back(
        {.negativeKey = negativeKey, .positiveKey = positiveKey});
  }
}

void InputMap::BindGamepadAxis(const std::string &axis, int gamepad,
                               GamepadAxis gamepadAxis, float deadzone) {
  auto &bindings = m_Axes[axis].gamepadBindings;

  const auto existing = std::find_if(
      bindings.begin(), bindings.end(),
      [gamepad, gamepadAxis](const GamepadAxisBinding &binding) {
        return binding.gamepad == gamepad && binding.axis == gamepadAxis;
      });

  if (existing != bindings.end()) {
    /*
     * Rebinding the same physical axis updates its deadzone rather than
     * creating a duplicate entry.
     */
    existing->deadzone = deadzone;
    return;
  }

  bindings.push_back(
      {.gamepad = gamepad, .axis = gamepadAxis, .deadzone = deadzone});
}

float InputMap::GetAxis(const std::string &axis) const {
  const auto iterator = m_Axes.find(axis);

  if (iterator == m_Axes.end())
    return 0.0f;

  const Axis &inputAxis = iterator->second;

  float value = 0.0f;

  /*
   * Digital bindings contribute full-strength values.
   *
   * Holding both directions cancels them out.
   */
  for (const auto &binding : inputAxis.keyBindings) {
    float digitalValue = 0.0f;

    if (::IsKeyDown(binding.negativeKey))
      digitalValue -= 1.0f;

    if (::IsKeyDown(binding.positiveKey))
      digitalValue += 1.0f;

    /*
     * Keep whichever binding currently has the greatest magnitude.
     * This prevents multiple keyboard bindings from adding beyond the
     * valid -1 to +1 range.
     */
    if (std::abs(digitalValue) > std::abs(value)) {
      value = digitalValue;
    }
  }

  for (const auto &binding : inputAxis.gamepadBindings) {
    if (!::IsGamepadAvailable(binding.gamepad))
      continue;

    float analogValue = ::GetGamepadAxisMovement(binding.gamepad, binding.axis);

    if (std::abs(analogValue) < binding.deadzone) {
      analogValue = 0.0f;
    }

    if (std::abs(analogValue) > std::abs(value)) {
      value = analogValue;
    }
  }

  return value;
}

bool InputMap::IsDown(const std::string &action) const {
  const auto iterator = m_Actions.find(action);

  if (iterator == m_Actions.end())
    return false;

  const Action &inputAction = iterator->second;

  for (const KeyboardKey key : inputAction.keys) {
    if (::IsKeyDown(key))
      return true;
  }

  for (const auto &binding : inputAction.gamepadButtons) {
    if (::IsGamepadAvailable(binding.gamepad) &&
        ::IsGamepadButtonDown(binding.gamepad, binding.button)) {
      return true;
    }
  }

  return false;
}

bool InputMap::IsPressed(const std::string &action) const {
  const auto iterator = m_Actions.find(action);

  if (iterator == m_Actions.end())
    return false;

  const Action &inputAction = iterator->second;

  for (const KeyboardKey key : inputAction.keys) {
    if (::IsKeyPressed(key))
      return true;
  }

  for (const auto &binding : inputAction.gamepadButtons) {
    if (::IsGamepadAvailable(binding.gamepad) &&
        ::IsGamepadButtonPressed(binding.gamepad, binding.button)) {
      return true;
    }
  }

  return false;
}

bool InputMap::IsReleased(const std::string &action) const {
  const auto iterator = m_Actions.find(action);

  if (iterator == m_Actions.end())
    return false;

  const Action &inputAction = iterator->second;

  for (const KeyboardKey key : inputAction.keys) {
    if (::IsKeyReleased(key))
      return true;
  }

  for (const auto &binding : inputAction.gamepadButtons) {
    if (::IsGamepadAvailable(binding.gamepad) &&
        ::IsGamepadButtonReleased(binding.gamepad, binding.button)) {
      return true;
    }
  }

  return false;
}

void InputMap::ClearAction(const std::string &action) {
  m_Actions.erase(action);
}

void InputMap::Clear() {
  m_Actions.clear();
  m_Axes.clear();
}
} // namespace Levye