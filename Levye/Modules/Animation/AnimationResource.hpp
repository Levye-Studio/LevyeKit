#pragma once

#include <raylib.h>

#include <Levye/Modules/Animation/AnimationAPI.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Levye {

/** @brief Owns raylib animation batches and exposes generation-safe handles. */
class AnimationResource {
 public:
  AnimationResource() = default;
  ~AnimationResource();
  AnimationResource(const AnimationResource&) = delete;
  AnimationResource& operator=(const AnimationResource&) = delete;
  AnimationResource(AnimationResource&&) = delete;
  AnimationResource& operator=(AnimationResource&&) = delete;

  /** @brief Loads every animation in a file; returns empty on failure. */
  [[nodiscard]] std::vector<AnimationResourceHandle> Load(const std::string& path);
  /** @brief Returns a borrowed pointer, invalidated by Unload or Clear. */
  [[nodiscard]] const ModelAnimation* Get(AnimationResourceHandle handle) const noexcept;
  /** @brief Checks whether a generation-safe resource handle is live. */
  [[nodiscard]] bool IsValid(AnimationResourceHandle handle) const noexcept;
  /** @brief Releases a handle; frees its batch when no handles remain. */
  [[nodiscard]] bool Unload(AnimationResourceHandle handle);
  /** @brief Frees every owned batch and invalidates all handles. */
  void Clear() noexcept;

 private:
  struct Batch {
    ModelAnimation* animations = nullptr;
    int count = 0;
    std::size_t live = 0;
    ~Batch();
  };
  struct Slot {
    std::shared_ptr<Batch> batch;
    int animationIndex = 0;
    std::uint32_t generation = 1;
    bool active = false;
  };
  std::vector<Slot> m_Slots;
  std::vector<std::uint32_t> m_FreeSlots;

  [[nodiscard]] const Slot* Find(AnimationResourceHandle handle) const noexcept;
  [[nodiscard]] static AnimationResourceHandle MakeHandle(std::uint32_t index,
                                                          std::uint32_t generation) noexcept;
  void Retire(std::uint32_t index) noexcept;
};

}  // namespace Levye
