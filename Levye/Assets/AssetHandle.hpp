#pragma once

#include <cstdint>

namespace Levye {
/**
 * @brief Lightweight identifier for a host-owned asset.
 *
 * AssetHandle can safely be stored by persistent game state because it
 * contains no pointers into reloadable game code.
 *
 * A value of zero represents an invalid asset.
 */
struct AssetHandle {
  std::uint64_t id = 0;

  /**
   * @brief Returns whether this handle refers to an asset.
   */
  bool IsValid() const { return id != 0; }

  /**
   * @brief Allows direct comparison between asset handles.
   */
  bool operator==(const AssetHandle &) const = default;
};
} // namespace Levye