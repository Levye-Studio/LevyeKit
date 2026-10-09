#pragma once

#include <Levye/Core/ModuleAPI.hpp>
#include <Levye/Core/Services.hpp>
#include <Levye/Modules/ImGui/ImGuiAPI.hpp>

namespace Levye {

/**
 * @brief Access to LevyeKit's optional Dear ImGui integration.
 *
 * This class does not wrap Dear ImGui widgets. Game code should
 * continue using ImGui::Begin(), ImGui::Text(), etc.
 */
class ImGuiModule {
 public:
  /**
   * @brief Returns whether Dear ImGui is available.
   *
   * @return true when the host has initialized the ImGui module.
   */
  [[nodiscard]] static bool IsAvailable() noexcept {
    const HostServices* host = Services::Get();

    if (!host || !host->GetModuleAPI) {
      return false;
    }

    const auto* api = static_cast<const ImGuiAPI*>(
        host->GetModuleAPI(host->context, ModuleID::ImGui, IMGUI_API_VERSION));

    return api && api->IsAvailable && api->IsAvailable(host->context);
  }

 private:
  ImGuiModule() = delete;
};

}  // namespace Levye