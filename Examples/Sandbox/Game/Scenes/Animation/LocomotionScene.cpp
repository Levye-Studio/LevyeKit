#include "LocomotionScene.hpp"

#include <rlgl.h>

#include <Levye/Input/Input.hpp>
#include <Levye/Modules/Animation/EditablePose.hpp>
#include <Levye/Modules/Animation/IK/EndEffectorOrientation.hpp>
#include <Levye/Modules/Animation/IK/MultiChainIK.hpp>
#include <Levye/Modules/Animation/TargetSmoothing.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Sandbox {

namespace {
constexpr float kPlatformMinX = 1.5f;
constexpr float kPlatformMaxX = 2.9f;
constexpr float kPlatformHalfDepth = 1.25f;
constexpr float kPlatformTop = 0.3f;
constexpr float kSlopeMinZ = 1.0f;
constexpr float kSlopeMaxZ = 3.0f;
constexpr float kSlopeMaxX = 2.0f;
constexpr float kBodyHeight = 0.84f;
Matrix RootMatrix(Vector3 position, float yaw, Quaternion tilt = QuaternionIdentity()) {
  const Quaternion heading = QuaternionFromAxisAngle({0, 1, 0}, yaw);
  Matrix result = QuaternionToMatrix(QuaternionNormalize(QuaternionMultiply(tilt, heading)));
  result.m12 = position.x;
  result.m13 = position.y;
  result.m14 = position.z;
  return result;
}

Matrix BodyRootMatrix(Vector3 position, float yaw, Quaternion tilt) {
  Matrix result = RootMatrix(position, yaw, tilt);
  const Vector3 rotatedCenter = Vector3Transform({0, kBodyHeight, 0}, result);
  const Vector3 desiredCenter = Vector3Add(position, {0, kBodyHeight, 0});
  result.m12 += desiredCenter.x - rotatedCenter.x;
  result.m13 += desiredCenter.y - rotatedCenter.y;
  result.m14 += desiredCenter.z - rotatedCenter.z;
  return result;
}

Transform At(Vector3 position) { return {position, QuaternionIdentity(), {1, 1, 1}}; }

const char* IKName(Levye::AnimationIKChainStatus status) {
  switch (status) {
    case Levye::AnimationIKChainStatus::Solved:
      return "SOLVED";
    case Levye::AnimationIKChainStatus::Disabled:
      return "OFF";
    case Levye::AnimationIKChainStatus::Invalid:
      return "INVALID";
    case Levye::AnimationIKChainStatus::Conflict:
      return "CONFLICT";
    case Levye::AnimationIKChainStatus::Failed:
      return "FAILED";
  }
  return "UNKNOWN";
}
const char* RegionName(Levye::SupportRegionKind kind) {
  switch (kind) {
    case Levye::SupportRegionKind::None:
      return "NONE";
    case Levye::SupportRegionKind::Point:
      return "POINT";
    case Levye::SupportRegionKind::Segment:
      return "SEGMENT";
    case Levye::SupportRegionKind::Polygon:
      return "POLYGON";
  }
  return "UNKNOWN";
}
}  // namespace

bool LocomotionScene::Initialize() {
  if (m_Ready) return true;
  return SetPreset(2);
}

bool LocomotionScene::SetPreset(int count) {
  const auto limbs = Levye::GaitPreset(count);
  if (!m_Gait.Configure(limbs)) return false;
  m_Targets.assign(limbs.size(), {});
  m_Joints.assign(limbs.size(), {});
  m_Results.assign(limbs.size(), {});
  m_ClampedTargets.assign(limbs.size(), {});
  m_EndErrors.assign(limbs.size(), 0);
  m_RootDistances.assign(limbs.size(), 0);
  m_Reachable.assign(limbs.size(), true);
  m_EndOrientation.assign(limbs.size(), QuaternionIdentity());
  m_SwingOrientationStart.assign(limbs.size(), QuaternionIdentity());
  m_LandingOrientation.assign(limbs.size(), QuaternionIdentity());
  m_RootPosition = {};
  m_RootVelocity = {};
  m_BodyOffset = {};
  m_BodyTilt = QuaternionIdentity();
  m_RootTransform = MatrixIdentity();
  m_Balance = {};
  m_Stabilizer.Reset();
  m_RootYaw = 0;
  m_TurnVelocity = 0;
  m_Walking = false;
  m_Settling = false;
  m_Ready = true;
  return true;
}

Levye::ContactSurfaceHit LocomotionScene::QueryTerrain(Vector3 prediction, int limb) const {
  Levye::ContactSurfaceHit hit{};
  hit.valid = true;
  hit.contactId = static_cast<std::uint64_t>(limb + 1);
  hit.chainId = hit.contactId;
  hit.position = prediction;
  if (prediction.x >= kPlatformMinX && prediction.x <= kPlatformMaxX &&
      std::abs(prediction.z) <= kPlatformHalfDepth) {
    hit.surfaceId = 2;
    hit.position.y = kPlatformTop;
    hit.normal = {0, 1, 0};
  } else if (prediction.x >= 0 && prediction.x <= kSlopeMaxX && prediction.z >= kSlopeMinZ &&
             prediction.z <= kSlopeMaxZ) {
    hit.surfaceId = 3;
    hit.position.y = 0.2f * (prediction.z - kSlopeMinZ);
    hit.normal = Vector3Normalize({0, 1, -0.2f});
  } else {
    hit.surfaceId = 1;
    hit.position.y = 0;
    hit.normal = {0, 1, 0};
  }
  return hit;
}

void LocomotionScene::Update(float deltaTime) {
  if (!m_Ready || !std::isfinite(deltaTime) || deltaTime < 0) return;
  if (Levye::Input::IsPressed("LocomotionBiped")) (void)SetPreset(2);
  if (Levye::Input::IsPressed("LocomotionQuadruped")) (void)SetPreset(4);
  if (Levye::Input::IsPressed("LocomotionHexapod")) (void)SetPreset(6);
  if (Levye::Input::IsPressed("LocomotionWalk")) {
    m_Walking = !m_Walking;
    m_Settling = !m_Walking;
  }
  if (Levye::Input::IsPressed("LocomotionPause")) m_Gait.SetPaused(!m_Gait.IsPaused());
  if (Levye::Input::IsPressed("LocomotionSpeedDown")) m_Cadence = std::max(0.4f, m_Cadence - 0.15f);
  if (Levye::Input::IsPressed("LocomotionSpeedUp")) m_Cadence = std::min(2.5f, m_Cadence + 0.15f);
  if (Levye::Input::IsPressed("LocomotionStrideDown"))
    m_StrideLength = std::max(0.1f, m_StrideLength - 0.05f);
  if (Levye::Input::IsPressed("LocomotionStrideUp"))
    m_StrideLength = std::min(0.65f, m_StrideLength + 0.05f);
  if (Levye::Input::IsPressed("LocomotionMoveSpeedDown"))
    m_MoveSpeed = std::max(0.2f, m_MoveSpeed - 0.1f);
  if (Levye::Input::IsPressed("LocomotionMoveSpeedUp"))
    m_MoveSpeed = std::min(1.4f, m_MoveSpeed + 0.1f);
  if (Levye::Input::IsPressed("LocomotionMarch")) m_MarchInPlace = !m_MarchInPlace;
  if (Levye::Input::IsPressed("LocomotionStabilization"))
    m_Stabilizer.SetEnabled(!m_Stabilizer.IsEnabled());
  if (Levye::Input::IsPressed("LocomotionStrengthDown"))
    m_StabilizationStrength = std::max(0.0f, m_StabilizationStrength - 0.1f);
  if (Levye::Input::IsPressed("LocomotionStrengthUp"))
    m_StabilizationStrength = std::min(1.0f, m_StabilizationStrength + 0.1f);
  if (Levye::Input::IsPressed("LocomotionSupportDebug")) m_ShowSupport = !m_ShowSupport;
  if (Levye::Input::IsPressed("LocomotionHeightDown"))
    m_StepHeight = std::max(0.0f, m_StepHeight - 0.05f);
  if (Levye::Input::IsPressed("LocomotionHeightUp"))
    m_StepHeight = std::min(0.8f, m_StepHeight + 0.05f);
  const float turn = (Levye::Input::IsDown("LocomotionTurnRight") ? 1.0f : 0.0f) -
                     (Levye::Input::IsDown("LocomotionTurnLeft") ? 1.0f : 0.0f);
  const float turnAlpha = 1 - std::exp(-8.0f * deltaTime);
  m_TurnVelocity += (turn - m_TurnVelocity) * turnAlpha;
  m_RootYaw += m_TurnVelocity * deltaTime;
  Vector3 localVelocity{(Levye::Input::IsDown("LocomotionRight") ? 1.0f : 0.0f) -
                            (Levye::Input::IsDown("LocomotionLeft") ? 1.0f : 0.0f),
                        0,
                        (Levye::Input::IsDown("LocomotionBack") ? 1.0f : 0.0f) -
                            (Levye::Input::IsDown("LocomotionForward") ? 1.0f : 0.0f)};
  if (Vector3LengthSqr(localVelocity) > 1) localVelocity = Vector3Normalize(localVelocity);
  const Vector3 desiredVelocity =
      Vector3Scale(Vector3RotateByAxisAngle(localVelocity, {0, 1, 0}, m_RootYaw), m_MoveSpeed);
  m_RootVelocity = Levye::SmoothTarget(m_RootVelocity, desiredVelocity, deltaTime, 8.0f);
  m_RootPosition = Vector3Add(m_RootPosition, Vector3Scale(m_RootVelocity, deltaTime));
  const bool moving = Vector3LengthSqr(m_RootVelocity) > 0.0036f;
  if (m_Walking && !moving && !m_MarchInPlace) {
    for (const auto& state : m_Gait.States())
      if (state.state == Levye::GaitPhase::Swing) m_Settling = true;
  }
  if (m_Walking && (moving || m_MarchInPlace)) m_Settling = false;
  const float frequency = (m_Walking && (moving || m_MarchInPlace)) || m_Settling ? m_Cadence : 0;
  if (!m_Gait.Advance(deltaTime, frequency)) return;
  if (m_Settling) {
    bool allStance = true;
    for (const auto& state : m_Gait.States())
      allStance = state.state == Levye::GaitPhase::Stance && allStance;
    if (allStance) m_Settling = false;
  }
  const auto& limbs = m_Gait.Limbs();
  const int count = static_cast<int>(limbs.size());
  std::vector<BoneInfo> bones(static_cast<std::size_t>(1 + 3 * count));
  std::vector<Transform> sample(bones.size());
  bones[0].parent = -1;
  sample[0] = At({0, 0, 0});
  for (int i = 0; i < count; ++i) {
    const int root = 1 + 3 * i;
    bones[static_cast<std::size_t>(root)].parent = 0;
    bones[static_cast<std::size_t>(root + 1)].parent = root;
    bones[static_cast<std::size_t>(root + 2)].parent = root + 1;
    for (int joint = 0; joint < 3; ++joint)
      std::snprintf(bones[static_cast<std::size_t>(root + joint)].name,
                    sizeof(bones[static_cast<std::size_t>(root + joint)].name), "L%dJ%d", i, joint);
    const Vector3 nominal = limbs[static_cast<std::size_t>(i)].nominalLocal;
    const float side = nominal.x < 0 ? -1.0f : 1.0f;
    sample[static_cast<std::size_t>(root)] = At({0.48f * side, 0.78f, nominal.z * 0.75f});
    sample[static_cast<std::size_t>(root + 1)] = At({0.68f * side, 0.40f, nominal.z + 0.27f});
    sample[static_cast<std::size_t>(root + 2)] = At({nominal.x, 0.02f, nominal.z});
  }
  ModelSkeleton skeleton{};
  skeleton.boneCount = static_cast<int>(bones.size());
  skeleton.bones = bones.data();
  Levye::EditablePose pose;
  if (!pose.Initialize(skeleton) || !pose.SetSample(sample)) return;
  const Matrix intentRootWorld = RootMatrix(m_RootPosition, m_RootYaw);
  std::vector<Levye::AnimationIKChainRequest> requests(limbs.size());
  Levye::ContactConfig contactConfig{};
  contactConfig.maxDistance = 2;
  contactConfig.acquireRate = 24;
  contactConfig.releaseRate = 12;
  contactConfig.releaseOnMiss = true;
  for (int i = 0; i < count; ++i) {
    const auto& limb = limbs[static_cast<std::size_t>(i)];
    const int root = 1 + 3 * i;
    const Vector3 animationEnd =
        Vector3Transform(sample[static_cast<std::size_t>(root + 2)].translation, intentRootWorld);
    const Vector3 nominal = Vector3Transform(limb.nominalLocal, intentRootWorld);
    const float stepDuration = (1.0f - limb.dutyFactor) / m_Cadence;
    const Vector3 prediction = Vector3Add(nominal, Vector3Scale(m_RootVelocity, stepDuration));
    // Limit the horizontal stride first, then sample the surface at the actual footprint.
    // Clamping a three-dimensional terrain hit can leave a foot floating between surfaces.
    auto hit = QueryTerrain(prediction, i);
    hit.position.y = nominal.y;
    Levye::LandingPlan plan{};
    if (!Levye::PlanLanding(intentRootWorld, limb.nominalLocal, m_RootVelocity, stepDuration,
                            m_StrideLength, {0, 1, 0}, hit, plan))
      return;
    const auto surface = QueryTerrain(plan.world, i);
    plan.world = surface.position;
    plan.normal = surface.normal;
    hit = surface;
    auto adjusted = limb;
    adjusted.stepHeight = m_StepHeight;
    adjusted.easing = {1.35f, 2.4f};
    Quaternion plannedOrientation{};
    if (!Levye::EndEffectorOrientation::SurfaceOrientation(plan.normal, {0, 0, 1}, {0, 1, 0},
                                                           {0, 0, 1}, plannedOrientation))
      return;
    const auto& phase = m_Gait.States()[static_cast<std::size_t>(i)];
    // Hold the touchdown footprint while contact is acquiring; the next swing may replan.
    if (phase.state == Levye::GaitPhase::Stance &&
        m_Targets[static_cast<std::size_t>(i)].contact.state == Levye::ContactState::Acquiring) {
      plan = m_Targets[static_cast<std::size_t>(i)].landing;
      hit = QueryTerrain(plan.world, i);
    }
    if (!m_Targets[static_cast<std::size_t>(i)].initialized)
      m_LandingOrientation[static_cast<std::size_t>(i)] = plannedOrientation;
    if (phase.stepStarts) {
      m_SwingOrientationStart[static_cast<std::size_t>(i)] =
          m_EndOrientation[static_cast<std::size_t>(i)];
      m_LandingOrientation[static_cast<std::size_t>(i)] = plannedOrientation;
    }
    hit.hasOrientation = true;
    hit.orientation =
        phase.stepEnds ? m_LandingOrientation[static_cast<std::size_t>(i)] : plannedOrientation;
    if (!m_Targets[static_cast<std::size_t>(i)].Update(
            adjusted, m_Gait.States()[static_cast<std::size_t>(i)], animationEnd, plan, hit,
            contactConfig, {0, 1, 0}, deltaTime))
      return;
  }
  std::array<Levye::SupportContact, 6> supports{};
  std::array<Levye::StabilizationReach, 6> reaches{};
  int supportCount = 0;
  for (int i = 0; i < count; ++i) {
    const auto& contact = m_Targets[static_cast<std::size_t>(i)].contact;
    if (m_Gait.States()[static_cast<std::size_t>(i)].state != Levye::GaitPhase::Stance ||
        contact.state != Levye::ContactState::Locked)
      continue;
    const int root = 1 + 3 * i;
    const Vector3 hip =
        Vector3Transform(sample[static_cast<std::size_t>(root)].translation, intentRootWorld);
    const float upper = Vector3Distance(sample[static_cast<std::size_t>(root)].translation,
                                        sample[static_cast<std::size_t>(root + 1)].translation);
    const float lower = Vector3Distance(sample[static_cast<std::size_t>(root + 1)].translation,
                                        sample[static_cast<std::size_t>(root + 2)].translation);
    supports[static_cast<std::size_t>(supportCount)] = {
        contact.contactId,   limbs[static_cast<std::size_t>(i)].id,
        contact.anchorWorld, contact.normalWorld,
        contact.weight,      true};
    reaches[static_cast<std::size_t>(supportCount)] = {
        contact.contactId,      hip, contact.anchorWorld, std::abs(upper - lower) + 0.025f,
        upper + lower - 0.025f, true};
    ++supportCount;
  }
  Levye::StabilizationConfig balanceConfig{};
  balanceConfig.nominalHeight = 0.84f;
  balanceConfig.minimumHeight = 0.5f;
  balanceConfig.maximumHeight = 1.1f;
  balanceConfig.maximumCorrection = 0.12f;
  balanceConfig.maximumTilt = 0.25f;
  balanceConfig.orientationWeight = 0.65f;
  balanceConfig.strength = m_StabilizationStrength;
  const Quaternion heading = QuaternionFromAxisAngle({0, 1, 0}, m_RootYaw);
  const Vector3 bodyReference = Vector3Add(m_RootPosition, {0, kBodyHeight, 0});
  if (!m_Stabilizer.Update(deltaTime, bodyReference, bodyReference, heading, supports.data(),
                           supportCount, reaches.data(), supportCount, balanceConfig, m_Balance))
    return;
  m_BodyOffset = m_Balance.positionOffsetWorld;
  m_BodyTilt = m_Balance.orientationAdjustment;
  const Matrix rootWorld =
      BodyRootMatrix(Vector3Add(m_RootPosition, m_BodyOffset), m_RootYaw, m_BodyTilt);
  m_RootTransform = rootWorld;
  const Matrix worldRoot = MatrixInvert(rootWorld);
  for (int i = 0; i < count; ++i) {
    const auto& limb = limbs[static_cast<std::size_t>(i)];
    const int root = 1 + 3 * i;
    auto& request = requests[static_cast<std::size_t>(i)];
    request.id = limb.chainId;
    request.root = root;
    request.middle = root + 1;
    request.end = root + 2;
    request.tolerance = 0.01f;
    const Vector3 hipWorld =
        Vector3Transform(sample[static_cast<std::size_t>(root)].translation, rootWorld);
    const float upper = Vector3Distance(sample[static_cast<std::size_t>(root)].translation,
                                        sample[static_cast<std::size_t>(root + 1)].translation);
    const float lower = Vector3Distance(sample[static_cast<std::size_t>(root + 1)].translation,
                                        sample[static_cast<std::size_t>(root + 2)].translation);
    Levye::LimbReach reach{};
    if (!Levye::EvaluateLimbReach(hipWorld, m_Targets[static_cast<std::size_t>(i)].targetWorld,
                                  {0, -1, 0}, upper, lower, 0.025f, reach))
      return;
    m_RootDistances[static_cast<std::size_t>(i)] = reach.rootDistance;
    m_Reachable[static_cast<std::size_t>(i)] = reach.reachable;
    m_ClampedTargets[static_cast<std::size_t>(i)] = reach.targetWorld;
    const Vector3 modelTarget = Vector3Transform(reach.targetWorld, worldRoot);
    request.target[0] = modelTarget.x;
    request.target[1] = modelTarget.y;
    request.target[2] = modelTarget.z;
    const Vector3 pole = Vector3Add(sample[static_cast<std::size_t>(root)].translation,
                                    {0.7f * (limb.nominalLocal.x < 0 ? -1.0f : 1.0f), 0, 0.5f});
    request.pole[0] = pole.x;
    request.pole[1] = pole.y;
    request.pole[2] = pole.z;
  }
  if (!Levye::MultiChainIK::Solve(pose, requests.data(), count, m_Results.data())) return;
  std::vector<Levye::AnimationOrientationRequest> rotations(limbs.size());
  std::vector<Levye::AnimationOrientationResult> rotationResults(limbs.size());
  const Quaternion inverseRoot = QuaternionInvert(m_Balance.bodyOrientationWorld);
  for (int i = 0; i < count; ++i) {
    const auto& phase = m_Gait.States()[static_cast<std::size_t>(i)];
    Quaternion desired = m_LandingOrientation[static_cast<std::size_t>(i)];
    if (phase.state == Levye::GaitPhase::Swing) {
      if (!Levye::SampleSwingOrientation(m_SwingOrientationStart[static_cast<std::size_t>(i)],
                                         desired, phase.progress, desired))
        return;
    } else if (m_Targets[static_cast<std::size_t>(i)].contact.state ==
                   Levye::ContactState::Locked &&
               m_Targets[static_cast<std::size_t>(i)].contact.hasOrientation) {
      desired = m_Targets[static_cast<std::size_t>(i)].contact.orientationWorld;
    }
    m_EndOrientation[static_cast<std::size_t>(i)] = desired;
    auto& request = rotations[static_cast<std::size_t>(i)];
    request.id = limbs[static_cast<std::size_t>(i)].chainId;
    request.bone = 1 + 3 * i + 2;
    const Quaternion model = QuaternionNormalize(QuaternionMultiply(inverseRoot, desired));
    request.desired[0] = model.x;
    request.desired[1] = model.y;
    request.desired[2] = model.z;
    request.desired[3] = model.w;
  }
  if (!Levye::EndEffectorOrientation::SolveBatch(pose, rotations.data(), count,
                                                 rotationResults.data()))
    return;
  for (int i = 0; i < count; ++i) {
    const int root = 1 + 3 * i;
    for (int joint = 0; joint < 3; ++joint) {
      Levye::AnimationBoneTransform bone{};
      if (!pose.Get(root + joint, Levye::AnimationBoneSpace::Model, bone)) return;
      m_Joints[static_cast<std::size_t>(i)][static_cast<std::size_t>(joint)] = Vector3Transform(
          {bone.translation[0], bone.translation[1], bone.translation[2]}, rootWorld);
    }
    m_Reachable[static_cast<std::size_t>(i)] =
        m_Reachable[static_cast<std::size_t>(i)] &&
        m_Results[static_cast<std::size_t>(i)].status == Levye::AnimationIKChainStatus::Solved &&
        m_Results[static_cast<std::size_t>(i)].ik.reachable != 0;
    m_EndErrors[static_cast<std::size_t>(i)] =
        Vector3Distance(m_Joints[static_cast<std::size_t>(i)][2],
                        m_Targets[static_cast<std::size_t>(i)].targetWorld);
  }
}

void LocomotionScene::Draw() const {
  if (!m_Ready) return;
  Camera3D camera{};
  camera.position = Vector3Add(m_RootPosition, {5.2f, 4.0f, 6.6f});
  camera.target = Vector3Add(m_RootPosition, {0, 0.6f, 0});
  camera.up = {0, 1, 0};
  camera.fovy = 48;
  camera.projection = CAMERA_PERSPECTIVE;
  BeginMode3D(camera);
  DrawGrid(20, 1);
  DrawCube({2.2f, kPlatformTop * 0.5f, 0}, kPlatformMaxX - kPlatformMinX, kPlatformTop,
           2 * kPlatformHalfDepth, BROWN);
  DrawCubeWires({2.2f, kPlatformTop * 0.5f, 0}, kPlatformMaxX - kPlatformMinX, kPlatformTop,
                2 * kPlatformHalfDepth, WHITE);
  const Vector3 slopeA{0, 0, 1};
  const Vector3 slopeB{0, 0.4f, 3};
  const Vector3 slopeC{2, 0.4f, 3};
  const Vector3 slopeD{2, 0, 1};
  DrawTriangle3D(slopeA, slopeC, slopeB, DARKGREEN);
  DrawTriangle3D(slopeA, slopeD, slopeC, DARKGREEN);
  const auto& limbs = m_Gait.Limbs();
  const float bodyDepth = limbs.size() == 2 ? 1.1f : limbs.size() == 4 ? 1.65f : 2.15f;
  rlPushMatrix();
  rlMultMatrixf(MatrixToFloat(m_RootTransform));
  DrawCube({0, 0.84f, 0}, 1.2f, 0.4f, bodyDepth, DARKBLUE);
  DrawCubeWires({0, 0.84f, 0}, 1.2f, 0.4f, bodyDepth, WHITE);
  rlPopMatrix();
  const Vector3 intentCenter = Vector3Add(m_RootPosition, {0, kBodyHeight, 0});
  const Vector3 bodyCenter = Vector3Transform({0, kBodyHeight, 0}, m_RootTransform);
  DrawSphere(intentCenter, 0.07f, YELLOW);
  DrawSphere(bodyCenter, 0.09f, SKYBLUE);
  DrawLine3D(intentCenter, bodyCenter, SKYBLUE);
  DrawLine3D(
      bodyCenter,
      Vector3Add(bodyCenter, Vector3Scale(Vector3RotateByQuaternion({0, 1, 0}, m_BodyTilt), 0.45f)),
      SKYBLUE);
  for (std::size_t i = 0; i < limbs.size(); ++i) {
    const auto& points = m_Joints[i];
    const bool swing = m_Gait.States()[i].state == Levye::GaitPhase::Swing;
    const Color color = swing ? ORANGE : SKYBLUE;
    DrawCylinderEx(points[0], points[1], 0.09f, 0.07f, 8, color);
    DrawCylinderEx(points[1], points[2], 0.07f, 0.05f, 8, color);
    DrawSphere(points[0], 0.13f, WHITE);
    DrawSphere(points[1], 0.09f, WHITE);
    DrawSphere(points[2], 0.10f, color);
    const Quaternion orientation = m_EndOrientation[i];
    DrawLine3D(points[2],
               Vector3Add(points[2],
                          Vector3Scale(Vector3RotateByQuaternion({0, 1, 0}, orientation), 0.28f)),
               GREEN);
    DrawLine3D(points[2],
               Vector3Add(points[2],
                          Vector3Scale(Vector3RotateByQuaternion({0, 0, 1}, orientation), 0.28f)),
               BLUE);
    DrawSphereWires(m_Targets[i].targetWorld, 0.13f, 10, 12, m_Reachable[i] ? color : RED);
    if (!m_Reachable[i]) DrawLine3D(m_Targets[i].targetWorld, m_ClampedTargets[i], RED);
    if (m_Targets[i].contact.state == Levye::ContactState::Locked)
      DrawSphereWires(m_Targets[i].contact.anchorWorld, 0.19f, 10, 12, GREEN);
    if (swing) {
      DrawSphere(m_Targets[i].swingStart, 0.08f, PINK);
      DrawSphere(m_Targets[i].landing.world, 0.09f, GREEN);
      Vector3 previous = m_Targets[i].swingStart;
      for (int segment = 1; segment <= 12; ++segment) {
        Levye::SwingSample sample{};
        if (Levye::SampleSwing(m_Targets[i].swingStart, m_Targets[i].landing.world, {0, 1, 0},
                               segment / 12.0f, m_Targets[i].swingHeight, m_Targets[i].swingEasing,
                               sample)) {
          DrawLine3D(previous, sample.position, YELLOW);
          previous = sample.position;
        }
      }
    }
  }
  if (m_ShowSupport) {
    const auto& support = m_Balance.support;
    for (int i = 0; i < support.hullCount; ++i) {
      const Vector3 point = support.hullWorld[static_cast<std::size_t>(i)];
      DrawSphere(point, 0.055f, PURPLE);
      if (support.hullCount > 1)
        DrawLine3D(point, support.hullWorld[static_cast<std::size_t>((i + 1) % support.hullCount)],
                   MAGENTA);
    }
    if (support.hullCount >= 3)
      for (int i = 1; i + 1 < support.hullCount; ++i)
        DrawTriangle3D(support.hullWorld[0], support.hullWorld[static_cast<std::size_t>(i)],
                       support.hullWorld[static_cast<std::size_t>(i + 1)], Fade(PURPLE, 0.25f));
    if (support.kind != Levye::SupportRegionKind::None) {
      DrawSphere(support.centroidWorld, 0.075f, MAGENTA);
      DrawSphere(support.projectedReferenceWorld, 0.075f, YELLOW);
      DrawLine3D(support.projectedReferenceWorld,
                 Vector3Add(support.projectedReferenceWorld, m_Balance.suggestedCorrectionWorld),
                 ORANGE);
    }
  }
  EndMode3D();
  DrawRectangle(12, 12, 625, 165 + static_cast<int>(limbs.size()) * 21, Fade(BLACK, 0.8f));
  DrawText("F10  Procedural Locomotion", 24, 20, 21, WHITE);
  DrawText("1/2/3: limbs | G walk | M march | P pause | WASD move | Q/E turn", 24, 49, 15,
           LIGHTGRAY);
  DrawText("-/=: cadence | ,/.: stride | ;/': root speed | [ / ]: lift", 24, 69, 15, LIGHTGRAY);
  DrawText(TextFormat("Phase %.2f | %s | %s | cadence %.2f/s | lift %.2f", m_Gait.GlobalPhase(),
                      m_Walking    ? "WALK"
                      : m_Settling ? "SETTLING"
                                   : "STOP",
                      m_Gait.IsPaused() ? "PAUSED" : "RUNNING", m_Cadence, m_StepHeight),
           24, 94, 16, WHITE);
  DrawText(TextFormat("Velocity (%.2f, %.2f, %.2f) | speed %.1f | stride %.2f", m_RootVelocity.x,
                      m_RootVelocity.y, m_RootVelocity.z, m_MoveSpeed, m_StrideLength),
           24, 115, 15, WHITE);
  DrawText("White: attachment/joint | Green: anchor/landing | Pink: liftoff | Red: out of reach",
           24, 135, 14, LIGHTGRAY);
  for (std::size_t i = 0; i < limbs.size(); ++i) {
    const auto& phase = m_Gait.States()[i];
    DrawText(TextFormat("L%zu %s p%.2f IK %s reach %s d%.2f err %.3f", i + 1,
                        phase.state == Levye::GaitPhase::Stance ? "STANCE" : "SWING", phase.phase,
                        IKName(m_Results[i].status), m_Reachable[i] ? "YES" : "NO",
                        m_RootDistances[i], m_EndErrors[i]),
             24, 162 + static_cast<int>(i) * 21, 14,
             !m_Reachable[i]                           ? RED
             : phase.state == Levye::GaitPhase::Stance ? SKYBLUE
                                                       : ORANGE);
  }
  const int panelX = GetScreenWidth() >= 1100 ? GetScreenWidth() - 440 : 12;
  const int panelY = GetScreenWidth() >= 1100 ? 12 : 320;
  DrawRectangle(panelX, panelY, 425, 160, Fade(BLACK, 0.8f));
  DrawText("Body stabilization", panelX + 12, panelY + 8, 20, WHITE);
  DrawText(TextFormat("B: %s | N/H strength %.1f | V: support %s",
                      m_Stabilizer.IsEnabled() ? "ON" : "OFF", m_StabilizationStrength,
                      m_ShowSupport ? "ON" : "OFF"),
           panelX + 12, panelY + 35, 15, LIGHTGRAY);
  DrawText(TextFormat("Support %d (%s) | margin %+.3f %s", m_Balance.support.activeCount,
                      RegionName(m_Balance.support.kind), m_Balance.support.stabilityMargin,
                      m_Balance.support.stable ? "STABLE" : "OUTSIDE"),
           panelX + 12, panelY + 61, 15, m_Balance.support.stable ? GREEN : ORANGE);
  DrawText(TextFormat("Height %+.3f | tilt %.1f deg | reach %s", m_BodyOffset.y,
                      m_Balance.tiltRadians * RAD2DEG,
                      m_Balance.reachFeasible ? "FEASIBLE" : "CONFLICT"),
           panelX + 12, panelY + 84, 15, m_Balance.reachFeasible ? WHITE : RED);
  DrawText(TextFormat("Applied (%.2f, %.2f, %.2f)", m_BodyOffset.x, m_BodyOffset.y, m_BodyOffset.z),
           panelX + 12, panelY + 107, 15, LIGHTGRAY);
  DrawText("Purple: support | Yellow: intent | Cyan: corrected body", panelX + 12, panelY + 133, 14,
           LIGHTGRAY);
}

}  // namespace Sandbox
