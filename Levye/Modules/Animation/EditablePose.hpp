#pragma once

#include <raylib.h>

#include <Levye/Modules/Animation/AnimationAPI.hpp>
#include <array>
#include <vector>

namespace Levye {

/** Copied hierarchy and mutable local pose; owned by the service or its caller. */
class EditablePose {
 public:
  [[nodiscard]] bool Initialize(const ModelSkeleton& skeleton);
  [[nodiscard]] bool Matches(const ModelSkeleton& skeleton) const;
  [[nodiscard]] int BoneCount() const noexcept { return static_cast<int>(m_Bones.size()); }
  [[nodiscard]] int Parent(int bone) const noexcept;
  [[nodiscard]] int Find(const char* name) const noexcept;
  [[nodiscard]] bool Name(int bone, char* output, int capacity) const noexcept;
  /** @brief Imports raylib's model-space keyframe transforms as editable locals. */
  [[nodiscard]] bool SetSample(const std::vector<Transform>& model);
  [[nodiscard]] bool Get(int bone, AnimationBoneSpace space, AnimationBoneTransform& output) const;
  [[nodiscard]] bool Set(int bone, AnimationBoneSpace space, const AnimationBoneTransform& value);
  [[nodiscard]] bool IsSampled() const noexcept { return m_Sampled; }
  [[nodiscard]] const std::vector<Transform>& Local() const noexcept { return m_Local; }
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
