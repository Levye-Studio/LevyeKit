#pragma once

#include <cstdint>

namespace Levye {

/**
 * @brief ABI version of LevyeKit's optional ImGui integration.
 *
 * This version describes the host-to-game interface, not the
 * version of Dear ImGui itself.
 */
inline constexpr std::uint32_t IMGUI_API_VERSION = 1;

/**
 * @brief Host-provided interface for the optional ImGui module.
 *
 * Dear ImGui widgets are called directly through ImGui::*.
 * This interface only exposes LevyeKit-specific integration.
 */
struct ImGuiAPI {
  std::uint32_t version = IMGUI_API_VERSION;

  /**
   * @brief Checks whether the host ImGui service is initialized.
   *
   * @param context Opaque host-owned context.
   * @return true when Dear ImGui is available.
   */
  bool (*IsAvailable)(void* context) = nullptr;
};

}  // namespace Levye