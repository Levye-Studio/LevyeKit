#include "CoreScene.hpp"

#include <string>

#include "../Input/InputScene.hpp"
namespace Sandbox {
void CoreScene::Initialize() {
  playerTexture = Levye::Assets::LoadTexture("player.png");

  audio.Initialize();
}
void CoreScene::Update(float deltaTime) {
  InputScene::Update();
  const float horizontal = Levye::Input::GetAxis("MoveX");
  const float vertical = Levye::Input::GetAxis("MoveY");

  playerPosition.x += horizontal * playerSpeed * deltaTime;
  playerPosition.y += vertical * playerSpeed * deltaTime;

  if (Levye::Input::IsPressed("Back")) {
    Levye::Screen::Set("Menu");

    return;
  }

  if (Levye::Input::IsPressed("NormalTime")) {
    Levye::Time::SetScale(1.0f);
  }

  if (Levye::Input::IsPressed("SlowTime")) {
    Levye::Time::SetScale(0.25f);
  }

  if (Levye::Input::IsPressed("FastTime")) {
    Levye::Time::SetScale(2.0f);
  }

  if (Levye::Input::IsPressed("Pause")) {
    const bool paused = Levye::Time::IsPaused();

    Levye::Time::SetPaused(!paused);
  }

  audio.Update();
}
void CoreScene::Draw() const {
  // Host asset handles remain valid across game library reloads.
  ClearBackground(DARKBLUE);
  const Texture2D* texture = Levye::Assets::GetTexture(playerTexture);

  if (texture) {
    DrawTexture(*texture, static_cast<int>(playerPosition.x), static_cast<int>(playerPosition.y),
                WHITE);
  } else {
    DrawCircleV(playerPosition, 30.0f, GOLD);
  }

  DrawText("Move: WASD / Arrows / Controller", 40, 40, 20, RAYWHITE);

  DrawText("ESC / Controller B: Menu", 40, 70, 20, LIGHTGRAY);
  DrawText("P: Pause time | M: Pause music | R: Resume music", 40, 100, 20, LIGHTGRAY);

  DrawText(Levye::Time::IsPaused() ? "PAUSED" : "RUNNING", 20, 20, 20, WHITE);
}
}  // namespace Sandbox
