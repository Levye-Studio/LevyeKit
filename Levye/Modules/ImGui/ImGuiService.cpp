#include "ImGuiService.hpp"

#include <rlImGui.h>

namespace Levye {

bool ImGuiService::Initialize() {
  if (m_Initialized) {
    return true;
  }

  rlImGuiSetup(true);

  m_Initialized = true;
  return true;
}

void ImGuiService::BeginFrame() {
  if (!m_Initialized) {
    return;
  }

  rlImGuiBegin();
}

void ImGuiService::EndFrame() {
  if (!m_Initialized) {
    return;
  }

  rlImGuiEnd();
}

void ImGuiService::Shutdown() {
  if (!m_Initialized) {
    return;
  }

  rlImGuiShutdown();

  m_Initialized = false;
}

void ImGuiService::SetCaptureSettings(const ImGuiCaptureSettings& settings) {
  m_CaptureSettings = settings;
}

ImGuiCaptureSettings ImGuiService::GetCaptureSettings() const {
  return m_CaptureSettings;
}

}  // namespace Levye