#pragma once

#include <raylib.h>

namespace IKScene {

void Initialize();
void Update(float deltaTime);
void Draw3D();
void DrawUI();
void Shutdown();

}  // namespace IKScene
