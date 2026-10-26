#include "AnimationStateMachine.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Levye {
namespace {

template <typename T>
bool Compare(T actual, T expected, AnimationComparison comparison) {
  switch (comparison) {
    case AnimationComparison::Equal:
      return actual == expected;
    case AnimationComparison::NotEqual:
      return actual != expected;
    case AnimationComparison::Greater:
      return actual > expected;
    case AnimationComparison::Less:
      return actual < expected;
    case AnimationComparison::GreaterOrEqual:
      return actual >= expected;
    case AnimationComparison::LessOrEqual:
      return actual <= expected;
    default:
      return false;
  }
}

bool ValidCondition(const AnimationCondition& condition) {
  if (!condition.parameter || !*condition.parameter) return false;
  switch (condition.type) {
    case AnimationParameterType::Bool:
      return condition.comparison == AnimationComparison::Equal ||
             condition.comparison == AnimationComparison::NotEqual;
    case AnimationParameterType::Float:
      return std::isfinite(condition.floatValue) &&
             condition.comparison != AnimationComparison::Triggered &&
             static_cast<std::uint32_t>(condition.comparison) <=
                 static_cast<std::uint32_t>(AnimationComparison::LessOrEqual);
    case AnimationParameterType::Int:
      return condition.comparison != AnimationComparison::Triggered &&
             static_cast<std::uint32_t>(condition.comparison) <=
                 static_cast<std::uint32_t>(AnimationComparison::LessOrEqual);
    case AnimationParameterType::Trigger:
      return condition.comparison == AnimationComparison::Triggered;
    default:
      return false;
  }
}

}  // namespace

bool AnimationStateMachine::CreatesReturnCycle(const std::string& name,
                                               const AnimationStateDefinition& definition) const {
  std::string next = definition.returnState;
  for (std::size_t steps = 0; !next.empty() && steps <= m_States.size(); ++steps) {
    if (next == name) return true;
    const auto it = m_States.find(next);
    if (it == m_States.end()) return false;
    next = it->second.returnState;
  }
  return !next.empty();
}

bool AnimationStateMachine::AddState(const std::string& name,
                                     const AnimationStateDefinition& definition) {
  if (name.empty() || definition.animation == INVALID_ANIMATION_RESOURCE ||
      !std::isfinite(definition.transitionDuration) || definition.transitionDuration < 0.0f ||
      !std::isfinite(definition.speed) || definition.speed < 0.0f ||
      CreatesReturnCycle(name, definition)) {
    return false;
  }

  m_States.insert_or_assign(name, definition);
  return true;
}

void AnimationStateMachine::RemoveTransitionsUsing(const std::string& name) {
  std::erase_if(m_Transitions, [&name](const auto& transition) {
    return transition.sourceState == name || transition.targetState == name;
  });
}

bool AnimationStateMachine::RemoveState(const std::string& name) {
  if (m_States.erase(name) == 0) return false;
  RemoveTransitionsUsing(name);
  for (auto& [stateName, definition] : m_States) {
    if (definition.returnState == name) definition.returnState.clear();
  }
  if (m_CurrentState == name) m_CurrentState.clear();
  return true;
}

const AnimationStateDefinition* AnimationStateMachine::FindState(const std::string& name) const {
  const auto it = m_States.find(name);
  return it != m_States.end() ? &it->second : nullptr;
}

void AnimationStateMachine::RemoveStatesUsing(AnimationResourceHandle animation) {
  for (auto it = m_States.begin(); it != m_States.end();) {
    if (it->second.animation == animation) {
      const std::string name = it->first;
      if (m_CurrentState == name) m_CurrentState.clear();
      it = m_States.erase(it);
      RemoveTransitionsUsing(name);
      for (auto& [stateName, definition] : m_States) {
        if (definition.returnState == name) definition.returnState.clear();
      }
    } else {
      ++it;
    }
  }
}

const std::string* AnimationStateMachine::GetReturnState() const {
  const auto* state = FindState(m_CurrentState);
  if (!state || state->looping || state->returnState.empty() || !FindState(state->returnState))
    return nullptr;
  return &state->returnState;
}

bool AnimationStateMachine::AddTransition(const AnimationTransitionRule& rule) {
  if (!rule.targetState || !*rule.targetState || !FindState(rule.targetState) ||
      (rule.sourceState && *rule.sourceState && !FindState(rule.sourceState)) || !rule.conditions ||
      rule.conditionCount <= 0 || rule.conditionCount > 64)
    return false;

  AnimationTransitionDefinition copy;
  if (rule.sourceState) copy.sourceState = rule.sourceState;
  copy.targetState = rule.targetState;
  copy.priority = rule.priority;
  copy.conditions.reserve(static_cast<std::size_t>(rule.conditionCount));
  for (int i = 0; i < rule.conditionCount; ++i) {
    const AnimationCondition& condition = rule.conditions[i];
    if (!ValidCondition(condition)) return false;
    copy.conditions.push_back({condition.parameter, condition.type, condition.comparison,
                               condition.boolValue, condition.floatValue, condition.intValue});
  }
  m_Transitions.push_back(std::move(copy));
  return true;
}

bool AnimationStateMachine::ConditionsMatch(const AnimationTransitionDefinition& transition) const {
  for (const auto& condition : transition.conditions) {
    const auto it = m_Parameters.find(condition.parameter);
    if (it == m_Parameters.end() || it->second.type != condition.type) return false;
    const Parameter& parameter = it->second;
    bool matched = false;
    switch (condition.type) {
      case AnimationParameterType::Bool:
        matched = Compare(parameter.boolValue, condition.boolValue, condition.comparison);
        break;
      case AnimationParameterType::Float:
        matched = Compare(parameter.floatValue, condition.floatValue, condition.comparison);
        break;
      case AnimationParameterType::Int:
        matched = Compare(parameter.intValue, condition.intValue, condition.comparison);
        break;
      case AnimationParameterType::Trigger:
        matched = parameter.triggered;
        break;
    }
    if (!matched) return false;
  }
  return true;
}

const AnimationTransitionDefinition* AnimationStateMachine::FindTransition(
    bool allowLowerPriority) const {
  const auto* current = FindState(m_CurrentState);
  if (!current) return nullptr;

  const AnimationTransitionDefinition* best = nullptr;
  for (const auto& transition : m_Transitions) {
    if (!transition.sourceState.empty() && transition.sourceState != m_CurrentState) continue;
    if (transition.targetState == m_CurrentState) continue;
    const auto* target = FindState(transition.targetState);
    if (!target || (!allowLowerPriority && target->priority < current->priority) ||
        !ConditionsMatch(transition))
      continue;
    if (!best || transition.priority > best->priority) best = &transition;
  }
  return best;
}

void AnimationStateMachine::ConsumeTriggers(const AnimationTransitionDefinition& transition) {
  for (const auto& condition : transition.conditions) {
    if (condition.type != AnimationParameterType::Trigger) continue;
    auto it = m_Parameters.find(condition.parameter);
    if (it != m_Parameters.end()) it->second.triggered = false;
  }
}

AnimationStateMachine::Parameter* AnimationStateMachine::GetOrCreateParameter(
    const std::string& name, AnimationParameterType type) {
  if (name.empty()) return nullptr;
  const auto [it, inserted] = m_Parameters.try_emplace(name);
  if (inserted) it->second.type = type;
  return it->second.type == type ? &it->second : nullptr;
}

bool AnimationStateMachine::SetBoolParameter(const std::string& name, bool value) {
  Parameter* parameter = GetOrCreateParameter(name, AnimationParameterType::Bool);
  if (!parameter) return false;
  parameter->boolValue = value;
  return true;
}

bool AnimationStateMachine::SetFloatParameter(const std::string& name, float value) {
  if (!std::isfinite(value)) return false;
  Parameter* parameter = GetOrCreateParameter(name, AnimationParameterType::Float);
  if (!parameter) return false;
  parameter->floatValue = value;
  return true;
}

bool AnimationStateMachine::SetIntParameter(const std::string& name, int value) {
  Parameter* parameter = GetOrCreateParameter(name, AnimationParameterType::Int);
  if (!parameter) return false;
  parameter->intValue = value;
  return true;
}

bool AnimationStateMachine::SetTrigger(const std::string& name) {
  Parameter* parameter = GetOrCreateParameter(name, AnimationParameterType::Trigger);
  if (!parameter) return false;
  parameter->triggered = true;
  return true;
}

bool AnimationStateMachine::ResetTrigger(const std::string& name) {
  const auto it = m_Parameters.find(name);
  if (it == m_Parameters.end() || it->second.type != AnimationParameterType::Trigger) return false;
  it->second.triggered = false;
  return true;
}

void AnimationStateMachine::Clear() {
  m_States.clear();
  m_Parameters.clear();
  m_Transitions.clear();
  m_CurrentState.clear();
}

}  // namespace Levye
