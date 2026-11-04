#pragma once

#include <raylib.h>

namespace Sandbox::IKScene {

void Initialize();
void Update(float deltaTime);
void Draw3D();
void DrawUI();
void Shutdown();

}  // namespace Sandbox::IKScene
