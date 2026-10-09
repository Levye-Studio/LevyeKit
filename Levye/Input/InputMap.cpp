#include "InputMap.hpp"

#include <algorithm>
#include <cmath>

namespace Levye {
void InputMap::BindKey(const std::string& action, KeyboardKey key) {
  auto& keys = m_Actions[action].keys;

  /*
   * Avoid duplicate bindings because an action only needs to query a
   * physical key once.
   */
  if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
    keys.push_back(key);
  }
}

void InputMap::BindMouseButton(const std::string& action, MouseButton button) {
  auto& buttons = m_Actions[action].mouseButtons;

  /*
   * Avoid duplicate bindings because an action only needs to query a
   * physical mouse button once.
   */
  if (std::find(buttons.begin(), buttons.end(), button) == buttons.end()) {
    buttons.push_back(button);
  }
}

Vector2 InputMap::GetMouseDelta() const { return m_MouseDelta; }

float InputMap::GetMouseWheel() const { return m_MouseWheel; }

void InputMap::BindGamepadButton(const std::string& action, int gamepad,
                                 GamepadButton button) {
  auto& buttons = m_Actions[action].gamepadButtons;

  const auto existing = std::find_if(
      buttons.begin(), buttons.end(),
      [gamepad, button](const GamepadButtonBinding& binding) {
        return binding.gamepad == gamepad && binding.button == button;
      });

  if (existing == buttons.end()) {
    buttons.push_back({.gamepad = gamepad, .button = button});
  }
}

void InputMap::BindKeyAxis(const std::string& axis, KeyboardKey negativeKey,
                           KeyboardKey positiveKey) {
  auto& bindings = m_Axes[axis].keyBindings;

  const auto existing =
      std::find_if(bindings.begin(), bindings.end(),
                   [negativeKey, positiveKey](const KeyAxisBinding& binding) {
                     return binding.negativeKey == negativeKey &&
                            binding.positiveKey == positiveKey;
                   });

  if (existing == bindings.end()) {
    bindings.push_back(
        {.negativeKey = negativeKey, .positiveKey = positiveKey});
  }
}

void InputMap::BindGamepadAxis(const std::string& axis, int gamepad,
                               GamepadAxis gamepadAxis, float deadzone) {
  deadzone = std::isfinite(deadzone) ? std::clamp(deadzone, 0.0f, 1.0f) : 0.15f;
  auto& bindings = m_Axes[axis].gamepadBindings;

  const auto existing = std::find_if(
      bindings.begin(), bindings.end(),
      [gamepad, gamepadAxis](const GamepadAxisBinding& binding) {
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

void InputMap::Update() {
  m_MouseDelta =
      m_MouseCaptured ? Vector2{0.0f, 0.0f} : m_Input.getMouseDelta();
  m_MouseWheel = m_MouseCaptured ? 0.0f : m_Input.getMouseWheelMove();

  if (!std::isfinite(m_MouseDelta.x)) {
    m_MouseDelta.x = 0.0f;
  }

  if (!std::isfinite(m_MouseDelta.y)) {
    m_MouseDelta.y = 0.0f;
  }

  if (!std::isfinite(m_MouseWheel)) {
    m_MouseWheel = 0.0f;
  }

  for (auto& [name, action] : m_Actions) {
    bool keyboardDown = false;
    bool mouseDown = false;
    bool gamepadDown = false;

    // Read physical keyboard state.
    for (const auto key : action.keys) {
      keyboardDown |= m_Input.isKeyDown(key);
    }

    // Read physical mouse state.
    for (const auto button : action.mouseButtons) {
      mouseDown |= m_Input.isMouseButtonDown(button);
    }

    // Read physical gamepad state.
    for (const auto& binding : action.gamepadButtons) {
      gamepadDown |=
          m_Input.isGamepadAvailable(binding.gamepad) &&
          m_Input.isGamepadButtonDown(binding.gamepad, binding.button);
    }

    // Determine which input sources gameplay can use.
    const bool keyboardAvailable = !m_KeyboardCaptured;
    const bool mouseAvailable = !m_MouseCaptured;

    const bool visibleKeyboard = keyboardAvailable && keyboardDown;
    const bool visibleMouse = mouseAvailable && mouseDown;

    const bool visibleDown = visibleKeyboard || visibleMouse || gamepadDown;

    // Detect physical transitions separately for each device category.
    const bool keyboardPressed =
        keyboardAvailable && keyboardDown && !action.keyboardDown;

    const bool mousePressed = mouseAvailable && mouseDown && !action.mouseDown;

    const bool gamepadPressed = gamepadDown && !action.gamepadDown;

    const bool keyboardReleased =
        keyboardAvailable && !keyboardDown && action.keyboardDown;

    const bool mouseReleased = mouseAvailable && !mouseDown && action.mouseDown;

    const bool gamepadReleased = !gamepadDown && action.gamepadDown;

    // A new physical press should only trigger an action press
    // if the action was not already visible to gameplay.
    action.pressed =
        !action.down && (keyboardPressed || mousePressed || gamepadPressed);

    // A physical release should only trigger an action release
    // if no available binding is still holding the action.
    action.released = action.down && !visibleDown &&
                      (keyboardReleased || mouseReleased || gamepadReleased);

    action.physicalDown = keyboardDown || mouseDown || gamepadDown;

    action.suppressed = action.physicalDown && !visibleDown;

    action.down = visibleDown;

    // Preserve physical state across UI capture transitions.
    action.keyboardDown = keyboardDown;
    action.mouseDown = mouseDown;
    action.gamepadDown = gamepadDown;
  }

  for (auto& [name, axis] : m_Axes) {
    bool negative = false;
    bool positive = false;
    for (const auto& binding : axis.keyBindings) {
      negative = m_Input.isKeyDown(binding.negativeKey) || negative;
      positive = m_Input.isKeyDown(binding.positiveKey) || positive;
    }
    float value = static_cast<float>(positive) - static_cast<float>(negative);
    for (const auto& binding : axis.gamepadBindings) {
      if (!m_Input.isGamepadAvailable(binding.gamepad)) continue;
      float analog =
          m_Input.getGamepadAxisMovement(binding.gamepad, binding.axis);
      if (!std::isfinite(analog)) continue;
      analog = std::clamp(analog, -1.0f, 1.0f);
      if (std::abs(analog) < binding.deadzone) analog = 0.0f;
      if (std::abs(analog) > std::abs(value)) value = analog;
    }
    axis.value = value;
  }
}

float InputMap::GetAxis(const std::string& axis) const {
  const auto iterator = m_Axes.find(axis);
  return iterator != m_Axes.end() ? iterator->second.value : 0.0f;
}

bool InputMap::IsDown(const std::string& action) const {
  const auto iterator = m_Actions.find(action);
  return iterator != m_Actions.end() && iterator->second.down;
}

bool InputMap::IsPressed(const std::string& action) const {
  const auto iterator = m_Actions.find(action);
  return iterator != m_Actions.end() && iterator->second.pressed;
}

bool InputMap::IsReleased(const std::string& action) const {
  const auto iterator = m_Actions.find(action);
  return iterator != m_Actions.end() && iterator->second.released;
}

void InputMap::SetInputCapture(bool keyboard, bool mouse) {
  m_KeyboardCaptured = keyboard;
  m_MouseCaptured = mouse;
}

bool InputMap::IsKeyboardCaptured() const { return m_KeyboardCaptured; }

bool InputMap::IsMouseCaptured() const { return m_MouseCaptured; }

void InputMap::ClearAction(const std::string& action) {
  m_Actions.erase(action);
}

void InputMap::Clear() {
  m_Actions.clear();
  m_Axes.clear();
  m_MouseDelta = {};
  m_MouseWheel = 0.0f;
}
}  // namespace Levye