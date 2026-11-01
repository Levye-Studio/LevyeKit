#pragma once

#include <Levye/Modules/Animation/Locomotion/Locomotion.hpp>
#include <Levye/Modules/Animation/Stabilization/BodyStabilization.hpp>
#include <array>
#include <vector>

/** @brief Caller-owned procedural rig demonstrating the shared multi-chain IK solver. */
class LocomotionScene {
 public:
  [[nodiscard]] bool Initialize();
  [[nodiscard]] bool ConfigurePreset(int count) { return SetPreset(count); }
  void Update(float deltaTime);
  void Draw() const;
  [[nodiscard]] bool IsReady() const noexcept { return m_Ready; }
  [[nodiscard]] int LimbCount() const noexcept { return static_cast<int>(m_Results.size()); }
  [[nodiscard]] float GlobalPhase() const noexcept { return m_Gait.GlobalPhase(); }
  [[nodiscard]] bool IsWalking() const noexcept { return m_Walking; }
  [[nodiscard]] Vector3 RootVelocity() const noexcept { return m_RootVelocity; }
  [[nodiscard]] Vector3 IntentRoot() const noexcept { return m_RootPosition; }
  [[nodiscard]] Vector3 AppliedBodyOffset() const noexcept { return m_BodyOffset; }
  [[nodiscard]] Vector3 DrawnBodyCenter() const noexcept {
    return Vector3Transform({0, 0.84f, 0}, m_RootTransform);
  }
  [[nodiscard]] const Levye::StabilizationResult& Balance() const noexcept { return m_Balance; }
  [[nodiscard]] bool StabilizationEnabled() const noexcept { return m_Stabilizer.IsEnabled(); }
  [[nodiscard]] const Levye::LocomotionTarget* Target(int index) const noexcept {
    return index >= 0 && index < LimbCount() ? &m_Targets[static_cast<std::size_t>(index)]
                                             : nullptr;
  }
  [[nodiscard]] bool IsReachable(int index) const noexcept {
    return index >= 0 && index < LimbCount() && m_Reachable[static_cast<std::size_t>(index)];
  }
  [[nodiscard]] int SolvedCount() const noexcept {
    int count = 0;
    for (const auto& result : m_Results)
      if (result.status == Levye::AnimationIKChainStatus::Solved) ++count;
    return count;
  }
  [[nodiscard]] int ReachableCount() const noexcept {
    int count = 0;
    for (bool reachable : m_Reachable)
      if (reachable) ++count;
    return count;
  }

 private:
  [[nodiscard]] bool SetPreset(int count);
  [[nodiscard]] Levye::ContactSurfaceHit QueryTerrain(Vector3 prediction, int limb) const;
  Levye::GaitScheduler m_Gait{};
  std::vector<Levye::LocomotionTarget> m_Targets;
  std::vector<std::array<Vector3, 3>> m_Joints;
  std::vector<Levye::AnimationIKChainResult> m_Results;
  std::vector<Vector3> m_ClampedTargets;
  std::vector<float> m_EndErrors;
  std::vector<float> m_RootDistances;
  std::vector<bool> m_Reachable;
  std::vector<Quaternion> m_EndOrientation;
  std::vector<Quaternion> m_SwingOrientationStart;
  std::vector<Quaternion> m_LandingOrientation;
  Levye::BodyStabilizer m_Stabilizer{};
  Levye::StabilizationResult m_Balance{};
  Matrix m_RootTransform = MatrixIdentity();
  Quaternion m_BodyTilt{0, 0, 0, 1};
  float m_StabilizationStrength = 1.0f;
  bool m_ShowSupport = true;
  Vector3 m_RootPosition{};
  Vector3 m_RootVelocity{};
  Vector3 m_BodyOffset{};
  float m_RootYaw = 0;
  float m_TurnVelocity = 0;
  float m_MoveSpeed = 0.8f;
  float m_Cadence = 1.3f;
  float m_StrideLength = 0.42f;
  float m_StepHeight = 0.20f;
  bool m_Walking = false;
  bool m_Settling = false;
  bool m_MarchInPlace = false;
  bool m_Ready = false;
};
