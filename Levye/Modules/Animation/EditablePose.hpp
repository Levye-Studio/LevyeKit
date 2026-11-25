#pragma once

#include <raylib.h>

#include <Levye/Modules/Animation/AnimationAPI.hpp>
#include <array>
#include <vector>

namespace Levye {

/** @brief Copied hierarchy and mutable local pose owned by the service or caller. */
class EditablePose {
 public:
  /** @brief Initializes the editable hierarchy from a model skeleton. */
  [[nodiscard]] bool Initialize(const ModelSkeleton& skeleton);
  /** @brief Checks whether the pose hierarchy matches a model skeleton. */
  [[nodiscard]] bool Matches(const ModelSkeleton& skeleton) const;
  /** @brief Returns the number of bones in the pose. */
  [[nodiscard]] int BoneCount() const noexcept {
    return static_cast<int>(m_Bones.size());
  }
  /** @brief Returns a bone's parent index, or a negative value for a root. */
  [[nodiscard]] int Parent(int bone) const noexcept;
  /** @brief Finds a bone by name; returns a negative value if absent. */
  [[nodiscard]] int Find(const char* name) const noexcept;
  /** @brief Copies a bone name into the caller's buffer when it fits. */
  [[nodiscard]] bool Name(int bone, char* output, int capacity) const noexcept;
  /** @brief Imports raylib's model-space keyframe transforms as editable locals. */
  [[nodiscard]] bool SetSample(const std::vector<Transform>& model);
  /** @brief Reads a bone transform in local or model space. */
  [[nodiscard]] bool Get(int bone, AnimationBoneSpace space, AnimationBoneTransform& output) const;
  /** @brief Sets a bone transform in local or model space. */
  [[nodiscard]] bool Set(int bone, AnimationBoneSpace space, const AnimationBoneTransform& value);
  /** @brief Reports whether an animation sample has been imported. */
  [[nodiscard]] bool IsSampled() const noexcept {
    return m_Sampled;
  }
  /** @brief Returns the current local-space transforms. */
  [[nodiscard]] const std::vector<Transform>& Local() const noexcept {
    return m_Local;
  }
  /** @brief Evaluates model-space transforms through the bone hierarchy. */
  [[nodiscard]] std::vector<Transform> ModelPose() const;

 private:
  struct Bone {
    std::array<char, 32> name{};
    int parent = -1;
  };
  [[nodiscard]] bool ModelTransform(int bone, Transform& output) const;
  std::vector<Bone> m_Bones;
  std::vector<Transform> m_Local;
  bool m_Sampled = false;
};

}  // namespace Levye
