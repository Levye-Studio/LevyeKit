#include "Application.hpp"

#include <raylib.h>

namespace Levye {
Application::Application(const ApplicationConfig &config,
                         const GameAPI &gameAPI)
    : m_Config(config), m_GameAPI(gameAPI) {}

Application::~Application() {}

void Application::Run() {
  InitWindow(m_Config.width, m_Config.height, m_Config.title.c_str());

  SetTargetFPS(m_Config.targetFPS);

  if (m_GameAPI.OnLoad)
    m_GameAPI.OnLoad(&m_GameState);

  while (!WindowShouldClose()) {
    const float deltaTime = GetFrameTime();

    if (m_GameAPI.OnUpdate)
      m_GameAPI.OnUpdate(&m_GameState, deltaTime);

    BeginDrawing();

    ClearBackground(BLACK);

    if (m_GameAPI.OnDraw)
      m_GameAPI.OnDraw(&m_GameState);

    EndDrawing();
  }

  if (m_GameAPI.OnUnload)
    m_GameAPI.OnUnload(&m_GameState);

  CloseWindow();
}
} // namespace Levye