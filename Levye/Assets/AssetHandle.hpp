#pragma once

#include <cstdint>

namespace Levye {
/**
 * @brief Identifies the type of resource referenced by an AssetHandle.
 *
 * Asset types allow host-owned resource managers to reject handles that
 * belong to another resource system.
 */
enum class AssetType : std::uint8_t {
  Invalid = 0,

  Texture,
  Shader,
  Sound,
  Music,
  Font
};

/**
 * @brief Lightweight identifier for a host-owned asset.
 *
 * AssetHandle contains no pointers into reloadable game code and can
 * therefore safely be stored inside persistent GameState.
 *
 * The combination of an ID and AssetType prevents a handle from being
 * accidentally used with the wrong resource manager.
 */
struct AssetHandle {
  std::uint64_t id = 0;
  AssetType type = AssetType::Invalid;

  /**
   * @brief Returns whether this handle refers to an asset.
   */
  bool IsValid() const { return id != 0 && type != AssetType::Invalid; }

  /**
   * @brief Returns whether this handle belongs to a specific asset type.
   *
   * @param expectedType Asset type to compare against.
   */
  bool IsType(AssetType expectedType) const {
    return IsValid() && type == expectedType;
  }

  /**
   * @brief Allows direct comparison between asset handles.
   */
  bool operator==(const AssetHandle &) const = default;
};
} // namespace Levye