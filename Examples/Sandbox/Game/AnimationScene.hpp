#pragma once

#include <raylib.h>

#include <Levye/Modules/Animation/Animation.hpp>
#include <Levye/Modules/Animation/Contact/ContactTargets.hpp>
#include <array>
#include <cstdint>

#include "LocomotionScene.hpp"
#include "OrientationScene.hpp"

/** @brief Eleven independent Sandbox views of animation and procedural motion. */
class AnimationScene {
 public:
  static constexpr int SceneCount = 11;

  /** @brief Loads one game-owned robot model and shared host-owned clips. */
  [[nodiscard]] bool Initialize(const char* modelPath);
  /** @brief Selects an independent verification scene without resetting its controller. */
  [[nodiscard]] bool SelectScene(int index);
  /** @brief Handles controls for the selected scene and applies its pose. */
  void Update(float deltaTime);
  /** @brief Draws the robot, scene controls, and live API diagnostics. */
  void Draw() const;
  /** @brief Checks all persistent handles after a game-module reload. */
  [[nodiscard]] bool AfterReload();
  /** @brief Releases the model, controllers, and animation resources. */
  void Shutdown();

  [[nodiscard]] bool IsReady() const noexcept { return m_Ready; }
  [[nodiscard]] int CurrentScene() const noexcept { return m_CurrentScene; }
  [[nodiscard]] const char* SceneName(int index) const noexcept;
  [[nodiscard]] bool SceneReady(int index) const noexcept;

 private:
  struct SceneState {
    Levye::AnimationControllerHandle controller = Levye::INVALID_ANIMATION_CONTROLLER;
    int selectedClip = 0;
    float crossfadeDuration = 0.75f;
    float position = 0.0f;
    bool lastOperationPassed = true;
    std::array<char, 128> diagnostic{"Ready"};
  };

  [[nodiscard]] int FindClip(const char* name) const;
  [[nodiscard]] bool ConfigureController(int scene);
  [[nodiscard]] bool ConfigureConditional(Levye::AnimationControllerHandle controller);
  [[nodiscard]] bool SelectClip(int scene, int clip);
  [[nodiscard]] bool SetNamedState(int scene, const char* name, bool force);
  [[nodiscard]] bool RecreateLifecycleController();
  [[nodiscard]] bool VerifyPersistentHandles() const;
  [[nodiscard]] bool DiscoverIKChain();
  [[nodiscard]] bool ReadIKJoints();
  [[nodiscard]] Camera3D IKCamera() const;
  void DrawIKHUD() const;
  void UpdateIK(float deltaTime);
  void UpdateContacts(float deltaTime);
  void DrawContacts() const;
  void Record(int scene, bool success, const char* operation);
  void UpdateParameters(int scene, float deltaTime);
  [[nodiscard]] const char* ClipName(Levye::AnimationResourceHandle handle) const;

  Model m_Model{};
  Levye::AnimationPoseHandle m_EditablePose = Levye::INVALID_ANIMATION_POSE;
  int m_SelectedBone = 0;
  float m_BoneAngle = 0.0f;
  Camera3D m_Camera{};
  std::array<Levye::AnimationResourceHandle, 64> m_Animations{};
  std::array<std::array<char, 48>, 64> m_Names{};
  std::array<SceneState, SceneCount> m_Scenes{};
  std::array<char, 1024> m_ModelPath{};
  int m_AnimationCount = 0;
  int m_CurrentScene = 0;
  std::uint32_t m_ReloadCount = 0;
  bool m_ReloadPassed = true;
  bool m_Ready = false;

  Levye::AnimationPoseHandle m_IKPose = Levye::INVALID_ANIMATION_POSE;
  Levye::AnimationPoseHandle m_ContactPose = Levye::INVALID_ANIMATION_POSE;
  Levye::ContactController m_Contacts[2]{};
  Levye::ContactConfig m_ContactConfig{};
  Levye::AnimationIKChainResult m_ContactResults[2]{};
  Vector3 m_ContactOrigins[2]{};
  Vector3 m_ContactBaseWorld[2]{};
  Vector3 m_ContactEndWorld[2]{};
  Vector3 m_ContactRootPosition{};
  float m_ContactErrors[2]{};
  int m_ContactSurface = 0;
  int m_ContactSelected = 0;
  bool m_ContactInitialized = false;
  bool m_ContactsEnabled = true;
  LocomotionScene m_Locomotion{};
  OrientationScene m_Orientation{};

  Vector3 m_IKTarget{0.5f, 1.5f, 0.0f};
  Vector3 m_IKPole{0.0f, 1.5f, 1.0f};

  int m_IKRoot = -1;
  int m_IKMiddle = -1;
  int m_IKEnd = -1;

  bool m_IKEnabled = true;
  bool m_IKSolved = false;
  bool m_IKReadback = false;
  bool m_IKReachable = false;
  float m_IKEndError = -1.0f;  // Negative until first entry samples the current arm pose.
  Vector3 m_IKJointPositions[3]{};
  Vector3 m_IKSmoothedTarget{0.5f, 1.5f, 0.0f};
  float m_IKWeight = 1.0f;
  float m_IKSmoothingResponse = 8.0f;
  bool m_IKConstraintEnabled = false;
  Levye::AnimationIKConstraint m_IKConstraint{0.34906585f, 2.44346095f};
  Levye::AnimationIKResult m_IKResult{};
  int m_IKSelectedChain = 0;
  int m_IK2Root = -1;
  int m_IK2Middle = -1;
  int m_IK2End = -1;
  Vector3 m_IK2Target{};
  Vector3 m_IK2Pole{};
  Vector3 m_IK2SmoothedTarget{};
  Vector3 m_IK2JointPositions[3]{};
  Levye::AnimationIKConstraint m_IK2Constraint{0.34906585f, 2.44346095f};
  Levye::AnimationIKResult m_IK2Result{};
  Levye::AnimationIKChainStatus m_IKStatus = Levye::AnimationIKChainStatus::Disabled;
  Levye::AnimationIKChainStatus m_IK2Status = Levye::AnimationIKChainStatus::Disabled;
  float m_IK2EndError = -1.0f;
};
