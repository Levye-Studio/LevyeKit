#pragma once

#include <raylib.h>

#include <Levye/Modules/Animation/Animation.hpp>
#include <array>
#include <cstdint>

/** @brief Seven independent Sandbox views of host-owned animation controllers. */
class AnimationScene {
 public:
  static constexpr int SceneCount = 7;

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
  void Record(int scene, bool success, const char* operation);
  void UpdateParameters(int scene, float deltaTime);
  [[nodiscard]] const char* ClipName(Levye::AnimationResourceHandle handle) const;

  Model m_Model{};
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
};
