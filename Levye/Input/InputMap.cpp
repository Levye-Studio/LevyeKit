#include "InputMap.hpp"

#include <algorithm>

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

void InputMap::Clear() { m_Actions.clear(); }
} // namespace Levye