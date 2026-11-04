#include "OrientationScene.hpp"

#include <raymath.h>
#include <rlgl.h>

#include <Levye/Input/Input.hpp>
#include <Levye/Modules/Animation/EditablePose.hpp>
#include <Levye/Modules/Animation/IK/EndEffectorOrientation.hpp>
#include <Levye/Modules/Animation/IK/MultiChainIK.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace Sandbox {

namespace {
Transform At(Vector3 position) { return {position, QuaternionIdentity(), {1, 1, 1}}; }
void Copy(Quaternion q, float out[4]) {
  out[0] = q.x;
  out[1] = q.y;
  out[2] = q.z;
  out[3] = q.w;
}
void Axes(Vector3 origin, Quaternion q, float size, unsigned char alpha) {
  const Vector3 axes[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
  const Color colors[3] = {RED, GREEN, BLUE};
  for (int i = 0; i < 3; ++i) {
    const Vector3 tip =
        Vector3Add(origin, Vector3Scale(Vector3RotateByQuaternion(axes[i], q), size));
    const Color color = Fade(colors[i], alpha / 255.0f);
    DrawCylinderEx(origin, tip, 0.012f, 0.012f, 6, color);
    DrawSphere(tip, 0.026f, color);
  }
}
void Pad(Vector3 origin, Quaternion q, Color color, bool ghost) {
  rlPushMatrix();
  rlTranslatef(origin.x, origin.y, origin.z);
  rlMultMatrixf(MatrixToFloat(QuaternionToMatrix(q)));
  if (ghost) {
    DrawCubeWires({0, 0, 0}, 0.48f, 0.065f, 0.32f, color);
    DrawCubeWires({0, 0, 0.21f}, 0.13f, 0.085f, 0.11f, color);
  } else {
    DrawCube({0, 0, 0}, 0.48f, 0.065f, 0.32f, color);
    DrawCubeWires({0, 0, 0}, 0.48f, 0.065f, 0.32f, WHITE);
    DrawCube({0, 0, 0.21f}, 0.13f, 0.085f, 0.11f, DARKBLUE);
  }
  rlPopMatrix();
}
const char* Status(Levye::AnimationOrientationStatus status) {
  switch (status) {
    case Levye::AnimationOrientationStatus::Solved:
      return "SOLVED";
    case Levye::AnimationOrientationStatus::Disabled:
      return "OFF";
    case Levye::AnimationOrientationStatus::Conflict:
      return "CONFLICT";
    case Levye::AnimationOrientationStatus::Invalid:
      return "INVALID";
    case Levye::AnimationOrientationStatus::Failed:
      return "FAILED";
  }
  return "UNKNOWN";
}
}  // namespace

bool OrientationScene::Initialize() {
  m_FrozenTargets = m_Targets;
  m_Ready = true;
  return true;
}

void OrientationScene::Update(float deltaTime) {
  if (!m_Ready || !std::isfinite(deltaTime) || deltaTime < 0) return;
  if (Levye::Input::IsPressed("OrientationFlat")) m_Surface = 0;
  if (Levye::Input::IsPressed("OrientationSlope")) m_Surface = 1;
  if (Levye::Input::IsPressed("OrientationRotate")) m_Surface = 2;
  if (Levye::Input::IsPressed("OrientationFlat") || Levye::Input::IsPressed("OrientationSlope") ||
      Levye::Input::IsPressed("OrientationRotate"))
    for (auto& contact : m_Contacts) contact.Reset();
  if (Levye::Input::IsPressed("OrientationWeightDown")) m_Weight = std::max(0.0f, m_Weight - 0.1f);
  if (Levye::Input::IsPressed("OrientationWeightUp")) m_Weight = std::min(1.0f, m_Weight + 0.1f);
  if (Levye::Input::IsPressed("OrientationWeightZero")) m_Weight = 0;
  if (Levye::Input::IsPressed("OrientationWeightFull")) m_Weight = 1;
  if (Levye::Input::IsPressed("OrientationConstraints")) m_Constrained = !m_Constrained;
  if (Levye::Input::IsPressed("OrientationFreeze")) {
    m_FreezePosition = !m_FreezePosition;
    if (m_FreezePosition) m_FrozenTargets = m_Targets;
  }
  m_Time += deltaTime;

  BoneInfo bones[7]{};
  std::vector<Transform> sample(7);
  bones[0].parent = -1;
  sample[0] = At({0, 0, 0});
  for (int i = 0; i < 2; ++i) {
    const int root = 1 + 3 * i;
    const float side = i == 0 ? -1.0f : 1.0f;
    bones[root].parent = 0;
    bones[root + 1].parent = root;
    bones[root + 2].parent = root + 1;
    for (int j = 0; j < 3; ++j)
      std::snprintf(bones[root + j].name, sizeof(bones[root + j].name), "L%dJ%d", i, j);
    sample[root] = At({side * 0.45f, 1.3f, 0});
    sample[root + 1] = At({side * 0.65f, 0.75f, 0.1f});
    sample[root + 2] = At({side * 0.85f, 0.20f, 0.2f});
  }
  ModelSkeleton skeleton{};
  skeleton.boneCount = 7;
  skeleton.bones = bones;
  Levye::EditablePose pose;
  if (!pose.Initialize(skeleton) || !pose.SetSample(sample)) return;
  Levye::AnimationIKChainRequest ik[2]{};
  Levye::AnimationOrientationRequest orientation[2]{};
  Levye::AnimationOrientationResult results[2]{};
  for (int i = 0; i < 2; ++i) {
    const int root = 1 + 3 * i;
    const float side = i == 0 ? -1.0f : 1.0f;
    const float slope = m_Surface == 1   ? side * 0.65f
                        : m_Surface == 2 ? 0.7f * std::sin(m_Time)
                                         : 0;
    const Quaternion surfaceRotation = QuaternionFromAxisAngle({0, 0, 1}, slope);
    const Vector3 normal = Vector3RotateByQuaternion({0, 1, 0}, surfaceRotation);
    m_Normals[i] = normal;
    Levye::ContactSurfaceHit hit{};
    hit.valid = true;
    hit.contactId = static_cast<std::uint64_t>(i + 1);
    hit.chainId = hit.contactId;
    hit.surfaceId = hit.contactId;
    hit.position = {side * 0.9f, 0.20f, 0.2f};
    hit.normal = normal;
    hit.orientation = surfaceRotation;
    hit.hasOrientation = true;
    Levye::ContactConfig config{};
    config.maxSlopeRadians = 1.5f;
    config.followSurface = m_Surface == 2;
    config.acquireRate = 16;
    Levye::ContactSurfaceTransform surface{};
    surface.valid = m_Surface == 2;
    surface.surfaceId = hit.surfaceId;
    surface.worldFromSurface = MatrixRotateZ(slope);
    if (!m_Contacts[i].Update(hit.position, hit, config, deltaTime, &surface)) return;
    const Vector3 target = m_FreezePosition ? m_FrozenTargets[i]
                           : m_Contacts[i].state == Levye::ContactState::Inactive
                               ? hit.position
                               : m_Contacts[i].targetWorld;
    m_Targets[i] = target;
    ik[i].id = hit.chainId;
    ik[i].root = root;
    ik[i].middle = root + 1;
    ik[i].end = root + 2;
    ik[i].target[0] = target.x;
    ik[i].target[1] = target.y;
    ik[i].target[2] = target.z;
    ik[i].pole[0] = side * 1.4f;
    ik[i].pole[1] = 0.8f;
    ik[i].pole[2] = 0.8f;
    const Quaternion desired = m_Contacts[i].state == Levye::ContactState::Locked
                                   ? m_Contacts[i].orientationWorld
                                   : surfaceRotation;
    m_Desired[i] = desired;
    orientation[i].id = hit.chainId;
    orientation[i].bone = root + 2;
    orientation[i].weight = m_Weight;
    orientation[i].constrained = m_Constrained;
    orientation[i].maxAngle = 0.35f;
    orientation[i].maxTwist = 0.25f;
    Copy(desired, orientation[i].desired);
  }
  Levye::AnimationIKChainResult ikResults[2]{};
  if (!Levye::MultiChainIK::Solve(pose, ik, 2, ikResults) ||
      !Levye::EndEffectorOrientation::SolveBatch(pose, orientation, 2, results))
    return;
  for (int i = 0; i < 2; ++i) {
    m_Results[i] = results[i];
    for (int j = 0; j < 3; ++j) {
      Levye::AnimationBoneTransform bone{};
      if (!pose.Get(1 + 3 * i + j, Levye::AnimationBoneSpace::Model, bone)) return;
      m_Joints[i][j] = {bone.translation[0], bone.translation[1], bone.translation[2]};
      if (j == 2)
        m_Applied[i] = {bone.rotation[0], bone.rotation[1], bone.rotation[2], bone.rotation[3]};
    }
  }
}

void OrientationScene::Draw() const {
  if (!m_Ready) return;
  Camera3D camera{{3.5f, 2.8f, 4.7f}, {0, 0.7f, 0}, {0, 1, 0}, 45, CAMERA_PERSPECTIVE};
  BeginMode3D(camera);
  DrawGrid(12, 1);
  DrawCube({0, 1.35f, 0}, 1.15f, 0.28f, 0.8f, DARKBLUE);
  for (int i = 0; i < 2; ++i) {
    const Vector3 platform = m_Targets[i];
    rlPushMatrix();
    rlTranslatef(platform.x, platform.y - 0.08f, platform.z);
    rlRotatef(std::atan2(-m_Normals[i].x, m_Normals[i].y) * RAD2DEG, 0, 0, 1);
    DrawCube({0, 0, 0}, 1.1f, 0.08f, 0.85f, i == 0 ? BROWN : DARKGREEN);
    DrawCubeWires({0, 0, 0}, 1.1f, 0.08f, 0.85f, WHITE);
    rlPopMatrix();
    const auto& joints = m_Joints[i];
    DrawCylinderEx(joints[0], joints[1], 0.075f, 0.06f, 8, SKYBLUE);
    DrawCylinderEx(joints[1], joints[2], 0.06f, 0.045f, 8, SKYBLUE);
    for (Vector3 point : joints) DrawSphere(point, 0.085f, WHITE);
    Pad(joints[2], m_Applied[i], i == 0 ? ORANGE : SKYBLUE, false);
    Axes(joints[2], m_Applied[i], 0.42f, 255);
    const Vector3 desiredPoint = Vector3Add(m_Targets[i], {0, 0, 0.62f});
    Pad(desiredPoint, m_Desired[i], YELLOW, true);
    Axes(desiredPoint, m_Desired[i], 0.34f, 210);
    DrawLine3D(joints[2], Vector3Add(joints[2], Vector3Scale(m_Normals[i], 0.55f)), YELLOW);
    DrawSphereWires(m_Targets[i], 0.14f, 10, 12, GREEN);
  }
  EndMode3D();
  DrawRectangle(12, 12, 670, 183, Fade(BLACK, 0.82f));
  DrawText("F11  End-Effector Orientation", 24, 20, 21, WHITE);
  DrawText("1 flat | 2 slope | 3 rotating | [ / ] weight | 0/9 endpoints", 24, 48, 16, LIGHTGRAY);
  DrawText("F freeze contact target | C toggle limits", 24, 70, 16, LIGHTGRAY);
  DrawText(TextFormat("Surface %d | weight %.1f | limits %s | position %s", m_Surface + 1, m_Weight,
                      m_Constrained ? "ON" : "OFF", m_FreezePosition ? "FROZEN" : "FOLLOWING"),
           24, 96, 17, WHITE);
  for (int i = 0; i < 2; ++i)
    DrawText(TextFormat("End %d  %s  angular error %5.1f deg  limited %s", i + 1,
                        Status(m_Results[i].status), m_Results[i].angularError * RAD2DEG,
                        m_Results[i].constrained ? "YES" : "NO"),
             24, 123 + 23 * i, 18,
             m_Results[i].status == Levye::AnimationOrientationStatus::Solved ? SKYBLUE : RED);
  DrawText("Solid pad + axes: actual   |   Yellow wire pad + axes: desired   |   RGB = XYZ", 24,
           171, 14, LIGHTGRAY);
}

}  // namespace Sandbox
