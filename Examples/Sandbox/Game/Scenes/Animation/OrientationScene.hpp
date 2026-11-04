#pragma once

#include <Levye/Modules/Animation/AnimationAPI.hpp>
#include <Levye/Modules/Animation/Contact/ContactTargets.hpp>
#include <array>

namespace Sandbox {

/** @brief Two independent procedural end effectors; all persistent data is plain caller-owned
 * state. */
class OrientationScene {
 public:
  [[nodiscard]] bool Initialize();
  void Update(float deltaTime);
  void Draw() const;
  [[nodiscard]] bool IsReady() const noexcept { return m_Ready; }
  [[nodiscard]] bool IsPositionFrozen() const noexcept { return m_FreezePosition; }
  [[nodiscard]] Vector3 ContactTarget(int index) const noexcept {
    return index >= 0 && index < 2 ? m_Targets[static_cast<std::size_t>(index)] : Vector3{};
  }
  [[nodiscard]] Quaternion AppliedOrientation(int index) const noexcept {
    return index >= 0 && index < 2 ? m_Applied[static_cast<std::size_t>(index)] : Quaternion{};
  }

 private:
  std::array<std::array<Vector3, 3>, 2> m_Joints{};
  std::array<Vector3, 2> m_Normals{};
  std::array<Quaternion, 2> m_Applied{};
  std::array<Quaternion, 2> m_Desired{};
  std::array<Vector3, 2> m_Targets{{{-0.9f, 0.2f, 0.2f}, {0.9f, 0.2f, 0.2f}}};
  std::array<Vector3, 2> m_FrozenTargets{};
  std::array<Levye::AnimationOrientationResult, 2> m_Results{};
  std::array<Levye::ContactController, 2> m_Contacts{};
  float m_Weight = 1;
  float m_Time = 0;
  int m_Surface = 0;
  bool m_Constrained = false;
  bool m_FreezePosition = false;
  bool m_Ready = false;
};

}  // namespace Sandbox
