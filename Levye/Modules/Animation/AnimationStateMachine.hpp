#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "AnimationAPI.hpp"

namespace Levye {

/** @brief Host-owned definition of a named animation state. */
struct AnimationStateDefinition {
  AnimationResourceHandle animation = INVALID_ANIMATION_RESOURCE;
  float transitionDuration = 0.25f;
  float speed = 1.0f;
  bool looping = true;
  std::string returnState;
  int priority = 0;
};

/** @brief Host-owned copy of a transition condition. */
struct AnimationConditionDefinition {
  std::string parameter;
  AnimationParameterType type = AnimationParameterType::Bool;
  AnimationComparison comparison = AnimationComparison::Equal;
  bool boolValue = false;
  float floatValue = 0.0f;
  int intValue = 0;
};

/** @brief Host-owned rule evaluated in priority and registration order. */
struct AnimationTransitionDefinition {
  std::string sourceState;
  std::string targetState;
  std::vector<AnimationConditionDefinition> conditions;
  int priority = 0;
};

/** @brief Stores one controller's states, parameters, and transition rules in the host. */
class AnimationStateMachine {
 public:
  explicit AnimationStateMachine(AnimationControllerHandle controller) : m_Controller(controller) {}

  /** @brief Registers or replaces a named state, rejecting return cycles. */
  [[nodiscard]] bool AddState(const std::string& name, const AnimationStateDefinition& definition);
  /** @brief Removes a state and its conditional transitions. */
  [[nodiscard]] bool RemoveState(const std::string& name);
  /** @brief Retrieves a registered state while the machine remains unchanged. */
  [[nodiscard]] const AnimationStateDefinition* FindState(const std::string& name) const;
  /** @brief Returns the active state's name. */
  [[nodiscard]] const std::string& GetCurrentState() const noexcept { return m_CurrentState; }
  /** @brief Returns the controller owned by this machine. */
  [[nodiscard]] AnimationControllerHandle GetController() const noexcept { return m_Controller; }
  /** @brief Records a successfully activated state. */
  void SetCurrentState(const std::string& name) { m_CurrentState = name; }
  /** @brief Removes states referencing an unloaded animation. */
  void RemoveStatesUsing(AnimationResourceHandle animation);
  /** @brief Returns a one-shot state's configured return target, if any. */
  [[nodiscard]] const std::string* GetReturnState() const;

  /** @brief Adds an ordered conditional transition from ABI-safe input. */
  [[nodiscard]] bool AddTransition(const AnimationTransitionRule& rule);
  /** @brief Removes all conditional transitions. */
  void ClearTransitions() { m_Transitions.clear(); }
  /** @brief Chooses the highest-priority matching rule, resolving ties by insertion order. */
  [[nodiscard]] const AnimationTransitionDefinition* FindTransition(bool allowLowerPriority) const;
  /** @brief Consumes trigger conditions after a transition succeeds. */
  void ConsumeTriggers(const AnimationTransitionDefinition& transition);

  /** @brief Sets a typed Boolean parameter. */
  [[nodiscard]] bool SetBoolParameter(const std::string& name, bool value);
  /** @brief Sets a typed finite float parameter. */
  [[nodiscard]] bool SetFloatParameter(const std::string& name, float value);
  /** @brief Sets a typed integer parameter. */
  [[nodiscard]] bool SetIntParameter(const std::string& name, int value);
  /** @brief Arms a typed trigger. */
  [[nodiscard]] bool SetTrigger(const std::string& name);
  /** @brief Disarms an existing trigger. */
  [[nodiscard]] bool ResetTrigger(const std::string& name);
  /** @brief Clears all states, parameters, and transitions. */
  void Clear();

 private:
  struct Parameter {
    AnimationParameterType type = AnimationParameterType::Bool;
    bool boolValue = false;
    float floatValue = 0.0f;
    int intValue = 0;
    bool triggered = false;
  };

  [[nodiscard]] Parameter* GetOrCreateParameter(const std::string& name,
                                                AnimationParameterType type);
  [[nodiscard]] bool ConditionsMatch(const AnimationTransitionDefinition& transition) const;
  [[nodiscard]] bool CreatesReturnCycle(const std::string& name,
                                        const AnimationStateDefinition& definition) const;
  void RemoveTransitionsUsing(const std::string& name);

  AnimationControllerHandle m_Controller = INVALID_ANIMATION_CONTROLLER;
  std::unordered_map<std::string, AnimationStateDefinition> m_States;
  std::unordered_map<std::string, Parameter> m_Parameters;
  std::vector<AnimationTransitionDefinition> m_Transitions;
  std::string m_CurrentState;
};

}  // namespace Levye
