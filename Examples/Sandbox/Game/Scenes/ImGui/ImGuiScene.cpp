#include "ImGuiScene.hpp"

#include <Levye/LevyeKit.hpp>
namespace Sandbox {
void ImGuiScene::Draw() {
  if (Levye::ImGuiModule::IsAvailable()) {
    ImGui::Begin("LevyeKit Debug");

    ImGui::Text("ImGui module registered!");
    ImGui::Text("Availability: true");
    ImGui::Text("Hot reload: true");

    ImGui::End();
  }
}
}  // namespace Sandbox
