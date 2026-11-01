#include "EditablePose.hpp"

#include <raymath.h>

#include <cmath>
#include <cstring>

namespace Levye {
namespace {
constexpr float MinScale = 1.0e-7f;

bool Finite(const AnimationBoneTransform& value) {
  for (float v : value.translation)
    if (!std::isfinite(v)) return false;
  for (float v : value.rotation)
    if (!std::isfinite(v)) return false;
  for (float v : value.scale)
    if (!std::isfinite(v) || std::abs(v) < MinScale) return false;
  const float lengthSquared =
      value.rotation[0] * value.rotation[0] + value.rotation[1] * value.rotation[1] +
      value.rotation[2] * value.rotation[2] + value.rotation[3] * value.rotation[3];
  return std::isfinite(lengthSquared) && lengthSquared > MinScale;
}

Transform ToRaylib(const AnimationBoneTransform& value) {
  return {{value.translation[0], value.translation[1], value.translation[2]},
          QuaternionNormalize(
              {value.rotation[0], value.rotation[1], value.rotation[2], value.rotation[3]}),
          {value.scale[0], value.scale[1], value.scale[2]}};
}

AnimationBoneTransform FromRaylib(const Transform& value) {
  AnimationBoneTransform result{};
  result.translation[0] = value.translation.x;
  result.translation[1] = value.translation.y;
  result.translation[2] = value.translation.z;
  result.rotation[0] = value.rotation.x;
  result.rotation[1] = value.rotation.y;
  result.rotation[2] = value.rotation.z;
  result.rotation[3] = value.rotation.w;
  result.scale[0] = value.scale.x;
  result.scale[1] = value.scale.y;
  result.scale[2] = value.scale.z;
  return result;
}

Transform Compose(const Transform& parent, const Transform& local) {
  return {Vector3Add(parent.translation,
                     Vector3RotateByQuaternion(Vector3Multiply(parent.scale, local.translation),
                                               parent.rotation)),
          QuaternionNormalize(QuaternionMultiply(parent.rotation, local.rotation)),
          Vector3Multiply(parent.scale, local.scale)};
}

bool Relative(const Transform& parent, Transform& child) {
  if (std::abs(parent.scale.x) < MinScale || std::abs(parent.scale.y) < MinScale ||
      std::abs(parent.scale.z) < MinScale)
    return false;
  const Quaternion inverse = QuaternionInvert(parent.rotation);
  child.translation = Vector3Divide(
      Vector3RotateByQuaternion(Vector3Subtract(child.translation, parent.translation), inverse),
      parent.scale);
  child.rotation = QuaternionNormalize(QuaternionMultiply(inverse, child.rotation));
  child.scale = Vector3Divide(child.scale, parent.scale);
  return true;
}
}  // namespace

bool EditablePose::Initialize(const ModelSkeleton& skeleton) {
  if (!skeleton.bones || skeleton.boneCount == 0 || skeleton.boneCount > 65535) return false;
  std::vector<Bone> bones(skeleton.boneCount);
  for (unsigned int i = 0; i < skeleton.boneCount; ++i) {
    const int parent = skeleton.bones[i].parent;
    if (parent < -1 || parent >= static_cast<int>(skeleton.boneCount) ||
        parent == static_cast<int>(i))
      return false;
    std::memcpy(bones[i].name.data(), skeleton.bones[i].name, bones[i].name.size());
    bones[i].name.back() = '\0';
    bones[i].parent = parent;
  }
  for (unsigned int i = 0; i < skeleton.boneCount; ++i) {
    int current = static_cast<int>(i);
    for (unsigned int steps = 0; current >= 0; ++steps) {
      if (steps >= skeleton.boneCount) return false;
      current = bones[static_cast<std::size_t>(current)].parent;
    }
  }
  m_Bones = std::move(bones);
  m_Local.clear();
  m_Sampled = false;
  return true;
}

bool EditablePose::Matches(const ModelSkeleton& skeleton) const {
  if (!skeleton.bones || skeleton.boneCount != m_Bones.size()) return false;
  for (unsigned int i = 0; i < skeleton.boneCount; ++i) {
    if (skeleton.bones[i].parent != m_Bones[i].parent ||
        std::strncmp(skeleton.bones[i].name, m_Bones[i].name.data(), 32) != 0)
      return false;
  }
  return true;
}

int EditablePose::Parent(int bone) const noexcept {
  return bone >= 0 && bone < BoneCount() ? m_Bones[static_cast<std::size_t>(bone)].parent : -1;
}

int EditablePose::Find(const char* name) const noexcept {
  if (!name) return -1;
  for (int i = 0; i < BoneCount(); ++i)
    if (std::strcmp(m_Bones[static_cast<std::size_t>(i)].name.data(), name) == 0) return i;
  return -1;
}

bool EditablePose::Name(int bone, char* output, int capacity) const noexcept {
  if (!output || capacity <= 0 || bone < 0 || bone >= BoneCount()) return false;
  const char* name = m_Bones[static_cast<std::size_t>(bone)].name.data();
  const auto length = std::strlen(name);
  if (length >= static_cast<std::size_t>(capacity)) return false;
  std::memcpy(output, name, length + 1);
  return true;
}

bool EditablePose::SetSample(const std::vector<Transform>& model) {
  if (model.size() != m_Bones.size()) return false;
  for (const auto& transform : model)
    if (!Finite(FromRaylib(transform))) return false;
  std::vector<Transform> local = model;
  for (int i = 0; i < BoneCount(); ++i) {
    const int parent = Parent(i);
    if (parent >= 0 &&
        !Relative(model[static_cast<std::size_t>(parent)], local[static_cast<std::size_t>(i)]))
      return false;
  }
  m_Local = std::move(local);
  m_Sampled = true;
  return true;
}

std::vector<Transform> EditablePose::ModelPose() const {
  std::vector<Transform> result;
  if (!m_Sampled) return result;
  result.resize(m_Bones.size());
  for (int i = 0; i < BoneCount(); ++i)
    if (!ModelTransform(i, result[static_cast<std::size_t>(i)])) return {};
  return result;
}

bool EditablePose::ModelTransform(int bone, Transform& output) const {
  if (!m_Sampled || bone < 0 || bone >= BoneCount()) return false;
  std::vector<int> chain;
  for (int current = bone; current >= 0; current = Parent(current)) chain.push_back(current);
  output = m_Local[static_cast<std::size_t>(chain.back())];
  for (auto it = chain.rbegin() + 1; it != chain.rend(); ++it)
    output = Compose(output, m_Local[static_cast<std::size_t>(*it)]);
  return true;
}

bool EditablePose::Get(int bone, AnimationBoneSpace space, AnimationBoneTransform& output) const {
  if (!m_Sampled || bone < 0 || bone >= BoneCount()) return false;
  Transform value{};
  if (space == AnimationBoneSpace::Local)
    value = m_Local[static_cast<std::size_t>(bone)];
  else if (space == AnimationBoneSpace::Model) {
    if (!ModelTransform(bone, value)) return false;
  } else
    return false;
  output = FromRaylib(value);
  return true;
}

bool EditablePose::Set(int bone, AnimationBoneSpace space, const AnimationBoneTransform& value) {
  if (!m_Sampled || bone < 0 || bone >= BoneCount() || !Finite(value)) return false;
  Transform local = ToRaylib(value);
  if (space == AnimationBoneSpace::Model) {
    const int parent = Parent(bone);
    if (parent >= 0) {
      Transform worldParent{};
      if (!ModelTransform(parent, worldParent) || std::abs(worldParent.scale.x) < MinScale ||
          std::abs(worldParent.scale.y) < MinScale || std::abs(worldParent.scale.z) < MinScale)
        return false;
      if (!Relative(worldParent, local)) return false;
    }
  } else if (space != AnimationBoneSpace::Local)
    return false;
  m_Local[static_cast<std::size_t>(bone)] = local;
  return true;
}
}  // namespace Levye
