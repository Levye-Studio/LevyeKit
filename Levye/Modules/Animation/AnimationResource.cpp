#include "AnimationResource.hpp"

#include <limits>
#include <utility>

namespace Levye {

AnimationResource::Batch::~Batch() {
  if (animations) UnloadModelAnimations(animations, count);
}

AnimationResource::~AnimationResource() { Clear(); }

AnimationResourceHandle AnimationResource::MakeHandle(std::uint32_t index,
                                                      std::uint32_t generation) noexcept {
  return (static_cast<std::uint64_t>(generation) << 32) | (static_cast<std::uint64_t>(index) + 1);
}

const AnimationResource::Slot* AnimationResource::Find(
    AnimationResourceHandle handle) const noexcept {
  if (!handle) return nullptr;
  const auto low = static_cast<std::uint32_t>(handle);
  const auto generation = static_cast<std::uint32_t>(handle >> 32);
  if (!low || !generation) return nullptr;
  const auto index = static_cast<std::size_t>(low - 1);
  if (index >= m_Slots.size()) return nullptr;
  const auto& slot = m_Slots[index];
  return slot.active && slot.generation == generation ? &slot : nullptr;
}

const ModelAnimation* AnimationResource::Get(AnimationResourceHandle handle) const noexcept {
  const Slot* slot = Find(handle);
  return slot ? &slot->batch->animations[slot->animationIndex] : nullptr;
}

bool AnimationResource::IsValid(AnimationResourceHandle handle) const noexcept {
  return Find(handle) != nullptr;
}

void AnimationResource::Retire(std::uint32_t index) noexcept {
  auto& slot = m_Slots[index];
  slot.active = false;
  slot.animationIndex = 0;
  slot.batch.reset();
  if (slot.generation != std::numeric_limits<std::uint32_t>::max()) {
    ++slot.generation;
    m_FreeSlots.push_back(index);
  }
}

std::vector<AnimationResourceHandle> AnimationResource::Load(const std::string& path) {
  if (path.empty()) return {};

  int count = 0;
  ModelAnimation* raw = LoadModelAnimations(path.c_str(), &count);
  if (!raw) return {};
  // Even an empty batch must be released through raylib.
  if (count <= 0) {
    UnloadModelAnimations(raw, count);
    return {};
  }

  // Allocate registry storage before accepting ownership of the batch.
  // A temporary RAII batch protects the raylib allocation on exceptions.
  std::shared_ptr<Batch> batch;
  try {
    batch = std::make_shared<Batch>();
  } catch (...) {
    UnloadModelAnimations(raw, count);
    throw;
  }
  batch->animations = raw;
  batch->count = count;

  const std::size_t n = static_cast<std::size_t>(count);
  const auto limit = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max());
  const auto additional = n > m_FreeSlots.size() ? n - m_FreeSlots.size() : 0;
  if (m_Slots.size() > limit || additional > limit - m_Slots.size()) {
    return {};
  }

  // Preallocate everything that can throw before publishing any handles.
  std::vector<AnimationResourceHandle> handles;
  handles.reserve(n);
  m_Slots.reserve(m_Slots.size() + additional);
  m_FreeSlots.reserve(m_FreeSlots.size() + n);

  for (int i = 0; i < count; ++i) {
    std::uint32_t index;
    if (!m_FreeSlots.empty()) {
      index = m_FreeSlots.back();
      m_FreeSlots.pop_back();
    } else {
      index = static_cast<std::uint32_t>(m_Slots.size());
      m_Slots.emplace_back();
    }
    auto& slot = m_Slots[index];
    slot.batch = batch;
    slot.animationIndex = i;
    slot.active = true;
    ++batch->live;
    handles.push_back(MakeHandle(index, slot.generation));
  }
  return handles;
}

bool AnimationResource::Unload(AnimationResourceHandle handle) {
  if (!Find(handle)) return false;
  const auto index = static_cast<std::uint32_t>(handle) - 1;
  auto& slot = m_Slots[index];
  --slot.batch->live;
  Retire(index);
  return true;
}

void AnimationResource::Clear() noexcept {
  m_FreeSlots.clear();
  for (std::size_t i = 0; i < m_Slots.size(); ++i) {
    auto& slot = m_Slots[i];
    if (slot.active) {
      slot.active = false;
      slot.batch.reset();
      slot.animationIndex = 0;
      if (slot.generation != std::numeric_limits<std::uint32_t>::max()) {
        ++slot.generation;
      }
    }
    if (slot.generation != std::numeric_limits<std::uint32_t>::max()) {
      m_FreeSlots.push_back(static_cast<std::uint32_t>(i));
    }
  }
}

}  // namespace Levye
