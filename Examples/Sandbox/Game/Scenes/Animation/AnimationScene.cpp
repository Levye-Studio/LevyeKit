#include "AnimationScene.hpp"

#include <raymath.h>

#include <Levye/Input/Input.hpp>
#include <Levye/Modules/Animation/TargetSmoothing.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <vector>

#include "IKSceneLogic.hpp"

namespace Sandbox {

namespace {
constexpr const char* SceneNames[AnimationScene::SceneCount] = {
    "Animation Playback",      "Crossfade Laboratory",    "State Machine",
    "Conditional Transitions", "One-shot Actions",        "Combined Gameplay Simulation",
    "Lifecycle & Hot Reload",  "Inverse Kinematics",      "Procedural Contacts",
    "Procedural Locomotion",   "End-Effector Orientation"};

bool AddRule(Levye::AnimationControllerHandle controller, const char* source, const char* target,
             int priority, std::initializer_list<Levye::AnimationCondition> conditions) {
  const Levye::AnimationTransitionRule rule{source, target, conditions.begin(),
                                            static_cast<int>(conditions.size()), priority};
  return Levye::Animation::AddTransition(controller, rule);
}

Levye::AnimationCondition BoolCondition(const char* name, bool value) {
  Levye::AnimationCondition condition{};
  condition.parameter = name;
  condition.type = Levye::AnimationParameterType::Bool;
  condition.boolValue = value;
  return condition;
}

Levye::AnimationCondition FloatCondition(const char* name, Levye::AnimationComparison comparison,
                                         float value) {
  Levye::AnimationCondition condition{};
  condition.parameter = name;
  condition.type = Levye::AnimationParameterType::Float;
  condition.comparison = comparison;
  condition.floatValue = value;
  return condition;
}

Levye::AnimationCondition IntCondition(const char* name, int value) {
  Levye::AnimationCondition condition{};
  condition.parameter = name;
  condition.type = Levye::AnimationParameterType::Int;
  condition.intValue = value;
  return condition;
}

Levye::AnimationCondition TriggerCondition(const char* name) {
  Levye::AnimationCondition condition{};
  condition.parameter = name;
  condition.type = Levye::AnimationParameterType::Trigger;
  condition.comparison = Levye::AnimationComparison::Triggered;
  return condition;
}

bool IsOneShot(const char* name) {
  return std::strcmp(name, "Robot_Jump") == 0 || std::strcmp(name, "Robot_Punch") == 0 ||
         std::strcmp(name, "Robot_Death") == 0 || std::strcmp(name, "Robot_Wave") == 0;
}

int StatePriority(const char* name) {
  if (std::strcmp(name, "Robot_Death") == 0) return 10;
  if (std::strcmp(name, "Robot_Punch") == 0) return 5;
  if (std::strcmp(name, "Robot_Jump") == 0) return 4;
  if (std::strcmp(name, "Robot_Wave") == 0) return 3;
  if (std::strcmp(name, "Robot_Idle") == 0) return 1;
  return 1;
}

void SetPreferredBend(const Vector3 joints[3], Levye::AnimationIKConstraint& constraint) {
  Vector3 axis = Vector3Subtract(joints[2], joints[0]);
  Vector3 bend = Vector3Subtract(joints[1], joints[0]);
  if (Vector3LengthSqr(axis) > 1.0e-10f) {
    axis = Vector3Normalize(axis);
    bend = Vector3Subtract(bend, Vector3Scale(axis, Vector3DotProduct(bend, axis)));
  }
  if (Vector3LengthSqr(bend) > 1.0e-10f) bend = Vector3Normalize(bend);
  constraint.preferredDirection[0] = bend.x;
  constraint.preferredDirection[1] = bend.y;
  constraint.preferredDirection[2] = bend.z;
  constraint.hasPreviousDirection = 0;
}

const char* ContactStateName(Levye::ContactState state) {
  switch (state) {
    case Levye::ContactState::Inactive:
      return "INACTIVE";
    case Levye::ContactState::Acquiring:
      return "ACQUIRING";
    case Levye::ContactState::Locked:
      return "LOCKED";
    case Levye::ContactState::Releasing:
      return "RELEASING";
  }
  return "UNKNOWN";
}

const char* ContactHitName(Levye::ContactHitStatus status) {
  switch (status) {
    case Levye::ContactHitStatus::NoHit:
      return "NO HIT";
    case Levye::ContactHitStatus::Accepted:
      return "ACCEPTED";
    case Levye::ContactHitStatus::InvalidNormal:
      return "BAD NORMAL";
    case Levye::ContactHitStatus::TooFar:
      return "TOO FAR";
    case Levye::ContactHitStatus::TooSteep:
      return "TOO STEEP";
    case Levye::ContactHitStatus::LowConfidence:
      return "LOW CONFIDENCE";
    case Levye::ContactHitStatus::InvalidInput:
      return "INVALID";
  }
  return "UNKNOWN";
}

const char* ContactIKName(Levye::AnimationIKChainStatus status) {
  switch (status) {
    case Levye::AnimationIKChainStatus::Disabled:
      return "DISABLED";
    case Levye::AnimationIKChainStatus::Invalid:
      return "INVALID";
    case Levye::AnimationIKChainStatus::Conflict:
      return "CONFLICT";
    case Levye::AnimationIKChainStatus::Solved:
      return "SOLVED";
    case Levye::AnimationIKChainStatus::Failed:
      return "FAILED";
  }
  return "UNKNOWN";
}

Vector3 ContactSurfacePoint(Vector3 origin, int surface) {
  const Vector3 offset = surface == 0   ? Vector3{-0.45f, -0.16f, 0}
                         : surface == 1 ? Vector3{0, 0.12f, 0.12f}
                                        : Vector3{0.45f, -0.08f, 0.1f};
  return Vector3Add(origin, offset);
}

Vector3 ContactSurfaceNormal(int surface) {
  return surface == 2 ? Vector3Normalize({0.45f, 1, 0}) : Vector3{0, 1, 0};
}
}  // namespace

const char* AnimationScene::SceneName(int index) const noexcept {
  return index >= 0 && index < SceneCount ? SceneNames[index] : "Unknown";
}

bool AnimationScene::SceneReady(int index) const noexcept {
  return m_Ready && index >= 0 && index < SceneCount &&
         Levye::Animation::IsControllerValid(m_Scenes[static_cast<std::size_t>(index)].controller);
}

int AnimationScene::FindClip(const char* name) const {
  if (!name) return -1;
  for (int i = 0; i < m_AnimationCount; ++i) {
    if (std::strcmp(m_Names[static_cast<std::size_t>(i)].data(), name) == 0) return i;
  }
  return -1;
}

const char* AnimationScene::ClipName(Levye::AnimationResourceHandle handle) const {
  for (int i = 0; i < m_AnimationCount; ++i) {
    if (m_Animations[static_cast<std::size_t>(i)] == handle)
      return m_Names[static_cast<std::size_t>(i)].data();
  }
  return "None";
}

bool AnimationScene::ConfigureConditional(Levye::AnimationControllerHandle controller) {
  using Levye::AnimationComparison;
  return AddRule(controller, nullptr, "Robot_Punch", 100, {TriggerCondition("Punch")}) &&
         AddRule(controller, nullptr, "Robot_Jump", 90, {TriggerCondition("Jump")}) &&
         AddRule(controller, "Robot_Idle", "Robot_Running", 20,
                 {BoolCondition("Moving", true),
                  FloatCondition("MoveSpeed", AnimationComparison::Greater, 0.7f)}) &&
         AddRule(controller, "Robot_Idle", "Robot_Walking", 10,
                 {BoolCondition("Moving", true),
                  FloatCondition("MoveSpeed", AnimationComparison::LessOrEqual, 0.7f)}) &&
         AddRule(controller, "Robot_Walking", "Robot_Running", 20,
                 {BoolCondition("Moving", true),
                  FloatCondition("MoveSpeed", AnimationComparison::Greater, 0.7f)}) &&
         AddRule(controller, "Robot_Running", "Robot_Walking", 10,
                 {BoolCondition("Moving", true),
                  FloatCondition("MoveSpeed", AnimationComparison::LessOrEqual, 0.7f)}) &&
         AddRule(controller, "Robot_Walking", "Robot_Idle", 10, {BoolCondition("Moving", false)}) &&
         AddRule(controller, "Robot_Running", "Robot_Idle", 10, {BoolCondition("Moving", false)}) &&
         AddRule(controller, "Robot_Idle", "Robot_Dance", 15, {IntCondition("Emote", 1)}) &&
         AddRule(controller, "Robot_Dance", "Robot_Idle", 10, {IntCondition("Emote", 0)});
}

bool AnimationScene::ConfigureController(int scene) {
  auto& view = m_Scenes[static_cast<std::size_t>(scene)];
  view.controller = Levye::Animation::CreateController();
  if (!Levye::Animation::IsControllerValid(view.controller)) return false;

  for (int i = 0; i < m_AnimationCount; ++i) {
    const char* name = m_Names[static_cast<std::size_t>(i)].data();
    const bool oneShot = IsOneShot(name);
    if (!Levye::Animation::AddState(view.controller, name,
                                    m_Animations[static_cast<std::size_t>(i)], 0.3f, 1.0f, !oneShot,
                                    oneShot ? "Robot_Idle" : nullptr, StatePriority(name)))
      return false;
  }

  if ((scene == 3 || scene == 5) && !ConfigureConditional(view.controller)) return false;

  const int idle = FindClip("Robot_Idle");
  if (idle < 0) return false;
  view.selectedClip = idle;
  if (scene == 0 || scene == 1) {
    return Levye::Animation::BindAnimation(view.controller,
                                           m_Animations[static_cast<std::size_t>(idle)]) &&
           Levye::Animation::PlayBound(view.controller);
  }
  return Levye::Animation::ForceState(view.controller, "Robot_Idle");
}

bool AnimationScene::Initialize(const char* modelPath) {
  if (m_Ready || !modelPath || !Levye::Animation::IsAvailable()) return false;
  if (std::strlen(modelPath) >= m_ModelPath.size()) return false;
  std::memcpy(m_ModelPath.data(), modelPath, std::strlen(modelPath) + 1);
  m_Model = LoadModel(modelPath);
  if (!IsModelValid(m_Model)) {
    Shutdown();
    return false;
  }

  const int count = Levye::Animation::GetAnimationCount(modelPath);
  if (count <= 0 || count > static_cast<int>(m_Animations.size()) ||
      Levye::Animation::LoadAnimations(modelPath, m_Animations.data(), count) != count) {
    Shutdown();
    return false;
  }
  m_AnimationCount = count;

  for (int i = 0; i < count; ++i) {
    if (!Levye::Animation::GetAnimationName(m_Animations[static_cast<std::size_t>(i)],
                                            m_Names[static_cast<std::size_t>(i)].data(),
                                            static_cast<int>(m_Names[0].size()))) {
      Shutdown();
      return false;
    }
  }
  for (const char* name : {"Robot_Idle", "Robot_Walking", "Robot_Running", "Robot_Dance",
                           "Robot_Jump", "Robot_Punch", "Robot_Death", "Robot_Wave"}) {
    if (FindClip(name) < 0) {
      Shutdown();
      return false;
    }
  }
  for (int scene = 0; scene < SceneCount; ++scene) {
    if (!ConfigureController(scene)) {
      Shutdown();
      return false;
    }
  }
  m_EditablePose = Levye::Animation::CreatePose(m_Scenes[0].controller, &m_Model);
  if (!Levye::Animation::IsPoseValid(m_EditablePose)) {
    Shutdown();
    return false;
  }

  m_IKPose = Levye::Animation::CreatePose(m_Scenes[7].controller, &m_Model);

  if (!Levye::Animation::IsPoseValid(m_IKPose)) {
    Shutdown();
    return false;
  }
  m_ContactPose = Levye::Animation::CreatePose(m_Scenes[8].controller, &m_Model);
  if (!Levye::Animation::IsPoseValid(m_ContactPose)) {
    Shutdown();
    return false;
  }
  (void)DiscoverIKChain();
  if (!m_Locomotion.Initialize()) {
    Shutdown();
    return false;
  }
  if (!m_Orientation.Initialize()) {
    Shutdown();
    return false;
  }

  m_SelectedBone = Levye::Animation::GetPoseBoneCount(m_EditablePose) > 1 ? 1 : 0;

  m_Camera.position = {10.0f, 6.0f, 12.0f};
  m_Camera.target = {0.0f, 1.5f, 0.0f};
  m_Camera.up = {0.0f, 1.0f, 0.0f};
  m_Camera.fovy = 45.0f;
  m_Camera.projection = CAMERA_PERSPECTIVE;
  m_Ready = true;
  return true;
}

bool AnimationScene::ReadIKJoints() {
  m_IKReadback = false;
  if (m_IKRoot < 0 || m_IKMiddle < 0 || m_IKEnd < 0) return false;
  const int indices[3] = {m_IKRoot, m_IKMiddle, m_IKEnd};
  for (int i = 0; i < 3; ++i) {
    Levye::AnimationBoneTransform bone{};
    if (!Levye::Animation::GetPoseBoneTransform(m_IKPose, indices[i],
                                                Levye::AnimationBoneSpace::Model, bone))
      return false;
    m_IKJointPositions[i] = {bone.translation[0], bone.translation[1], bone.translation[2]};
  }
  if (m_IK2Root >= 0 && m_IK2Middle >= 0 && m_IK2End >= 0) {
    const int second[3] = {m_IK2Root, m_IK2Middle, m_IK2End};
    for (int i = 0; i < 3; ++i) {
      Levye::AnimationBoneTransform bone{};
      if (!Levye::Animation::GetPoseBoneTransform(m_IKPose, second[i],
                                                  Levye::AnimationBoneSpace::Model, bone))
        return false;
      m_IK2JointPositions[i] = {bone.translation[0], bone.translation[1], bone.translation[2]};
    }
  }
  m_IKReadback = true;
  return true;
}

bool AnimationScene::DiscoverIKChain() {
  m_IKRoot = m_IKMiddle = m_IKEnd = -1;
  m_IK2Root = m_IK2Middle = m_IK2End = -1;
  if (!Levye::Animation::SamplePose(m_IKPose)) {
    Record(7, false, "Could not sample robot pose for IK discovery");
    return false;
  }
  std::vector<IKSceneLogic::Bone> bones;
  const int count = Levye::Animation::GetPoseBoneCount(m_IKPose);
  bones.reserve(static_cast<std::size_t>(count));
  for (int i = 0; i < count; ++i) {
    char name[32]{};
    Levye::AnimationBoneTransform transform{};
    if (!Levye::Animation::GetPoseBoneName(m_IKPose, i, name, sizeof(name)) ||
        !Levye::Animation::GetPoseBoneTransform(m_IKPose, i, Levye::AnimationBoneSpace::Model,
                                                transform)) {
      Record(7, false, "Could not read robot skeleton");
      return false;
    }
    bones.push_back(
        {name,
         Levye::Animation::GetPoseBoneParent(m_IKPose, i),
         {transform.translation[0], transform.translation[1], transform.translation[2]}});
  }
  const auto chain = IKSceneLogic::FindChain(bones);
  if (!chain.Valid()) {
    Record(7, false, "No nondegenerate two-bone chain in robot");
    return false;
  }
  m_IKRoot = chain.root;
  m_IKMiddle = chain.middle;
  m_IKEnd = chain.end;
  const auto second = IKSceneLogic::FindChain(bones, chain);
  if (second.Valid() && IKSceneLogic::ResetTarget(bones[static_cast<std::size_t>(second.root)],
                                                  bones[static_cast<std::size_t>(second.middle)],
                                                  bones[static_cast<std::size_t>(second.end)],
                                                  m_IK2Target, m_IK2Pole)) {
    m_IK2Root = second.root;
    m_IK2Middle = second.middle;
    m_IK2End = second.end;
    m_IK2SmoothedTarget = m_IK2Target;
  }
  if (!IKSceneLogic::ResetTarget(bones[static_cast<std::size_t>(chain.root)],
                                 bones[static_cast<std::size_t>(chain.middle)],
                                 bones[static_cast<std::size_t>(chain.end)], m_IKTarget,
                                 m_IKPole)) {
    m_IKRoot = m_IKMiddle = m_IKEnd = -1;
    Record(7, false, "IK chain cannot initialize a reachable target");
    return false;
  }
  Record(7, true, "IK chain discovered");
  m_IKSmoothedTarget = m_IKTarget;
  if (!ReadIKJoints()) return false;
  SetPreferredBend(m_IKJointPositions, m_IKConstraint);
  if (m_IK2Root >= 0) SetPreferredBend(m_IK2JointPositions, m_IK2Constraint);
  return true;
}

void AnimationScene::UpdateContacts(float deltaTime) {
  if (Levye::Input::IsPressed("ContactSelect")) m_ContactSelected = 1 - m_ContactSelected;
  if (Levye::Input::IsPressed("ContactToggle")) m_ContactsEnabled = !m_ContactsEnabled;
  if (Levye::Input::IsPressed("ContactSurfacePrevious"))
    m_ContactSurface = (m_ContactSurface + 2) % 3;
  if (Levye::Input::IsPressed("ContactSurfaceNext")) m_ContactSurface = (m_ContactSurface + 1) % 3;
  if (Levye::Input::IsPressed("ContactRelease")) m_Contacts[m_ContactSelected].Release();
  if (Levye::Input::IsPressed("AnimationRestart")) {
    for (auto& contact : m_Contacts) contact.Reset();
    m_ContactRootPosition = {};
    m_ContactInitialized = false;
  }
  if (Levye::Input::IsDown("AnimationLeft")) m_ContactRootPosition.x -= deltaTime;
  if (Levye::Input::IsDown("AnimationRight")) m_ContactRootPosition.x += deltaTime;
  if (!Levye::Animation::SamplePose(m_ContactPose)) {
    Record(8, false, "Sample contact pose");
    return;
  }
  const int ends[2] = {m_IKEnd, m_IK2End};
  const int roots[2] = {m_IKRoot, m_IK2Root};
  const int middles[2] = {m_IKMiddle, m_IK2Middle};
  const Vector3 poles[2] = {m_IKPole, m_IK2Pole};
  if (roots[0] < 0 || roots[1] < 0) {
    Record(8, false, "Two independent chains required");
    return;
  }
  auto toWorld = [&](Vector3 point) {
    return Vector3Add(Vector3Transform(point, m_Model.transform), m_ContactRootPosition);
  };
  Levye::AnimationIKChainRequest requests[2]{};
  for (int i = 0; i < 2; ++i) {
    Levye::AnimationBoneTransform bone{};
    if (!Levye::Animation::GetPoseBoneTransform(m_ContactPose, ends[i],
                                                Levye::AnimationBoneSpace::Model, bone)) {
      Record(8, false, "Read contact bone");
      return;
    }
    m_ContactBaseWorld[i] =
        toWorld({bone.translation[0], bone.translation[1], bone.translation[2]});
    if (!m_ContactInitialized) m_ContactOrigins[i] = m_ContactBaseWorld[i];
  }
  m_ContactInitialized = true;
  m_ContactConfig.maxDistance = 2.5f;
  m_ContactConfig.maxSlopeRadians = 0.9f;
  m_ContactConfig.releaseOnMiss = true;
  m_ContactConfig.reacquireOnSurfaceChange = true;
  for (int i = 0; i < 2; ++i) {
    const Vector3 point = ContactSurfacePoint(m_ContactOrigins[i], m_ContactSurface);
    const Vector3 normal = ContactSurfaceNormal(m_ContactSurface);
    Levye::ContactSurfaceHit hit{};
    hit.valid = m_ContactsEnabled;
    hit.contactId = static_cast<std::uint64_t>(101 + i);
    hit.chainId = static_cast<std::uint64_t>(i + 1);
    hit.surfaceId = static_cast<std::uint64_t>(10 * i + m_ContactSurface + 1);
    hit.position = point;
    hit.normal = normal;
    if (!m_Contacts[i].Update(m_ContactBaseWorld[i], hit, m_ContactConfig, deltaTime))
      Record(8, false, "Update contact");
    auto& request = requests[i];
    request.id = static_cast<std::uint64_t>(i + 1);
    request.root = roots[i];
    request.middle = middles[i];
    request.end = ends[i];
    request.pole[0] = poles[i].x;
    request.pole[1] = poles[i].y;
    request.pole[2] = poles[i].z;
    request.tolerance = 0.01f;
    Levye::AnimationIKChainRequest converted{};
    if (Levye::ContactToIKRequest(m_Contacts[i], request, m_Model.transform, m_ContactRootPosition,
                                  converted))
      request = converted;
    else
      request.enabled = 0;
  }
  if (!Levye::Animation::SolveIKChains(m_ContactPose, requests, 2, m_ContactResults))
    Record(8, false, "Solve contact batch");
  if (!Levye::Animation::ApplyPose(m_ContactPose, &m_Model)) Record(8, false, "Apply contact pose");
  for (int i = 0; i < 2; ++i) {
    Levye::AnimationBoneTransform bone{};
    if (Levye::Animation::GetPoseBoneTransform(m_ContactPose, ends[i],
                                               Levye::AnimationBoneSpace::Model, bone)) {
      m_ContactEndWorld[i] =
          toWorld({bone.translation[0], bone.translation[1], bone.translation[2]});
      m_ContactErrors[i] = Vector3Distance(m_ContactEndWorld[i], m_Contacts[i].targetWorld);
    }
  }
}

void AnimationScene::UpdateIK(float deltaTime) {
  if (Levye::Input::IsPressed("IKSelectChain") && m_IK2Root >= 0)
    m_IKSelectedChain = 1 - m_IKSelectedChain;
  if (Levye::Input::IsPressed("IKToggle")) m_IKEnabled = !m_IKEnabled;
  if (Levye::Input::IsPressed("IKWeightDown")) m_IKWeight = std::max(0.0f, m_IKWeight - 0.1f);
  if (Levye::Input::IsPressed("IKWeightUp")) m_IKWeight = std::min(1.0f, m_IKWeight + 0.1f);
  if (Levye::Input::IsPressed("IKResponseDown"))
    m_IKSmoothingResponse = std::max(0.0f, m_IKSmoothingResponse - 1.0f);
  if (Levye::Input::IsPressed("IKResponseUp"))
    m_IKSmoothingResponse = std::min(30.0f, m_IKSmoothingResponse + 1.0f);
  constexpr float bendStep = 0.0872664626f;
  constexpr float pi = 3.14159265358979323846f;
  auto& limits = m_IKSelectedChain ? m_IK2Constraint : m_IKConstraint;
  if (Levye::Input::IsPressed("IKConstraintToggle")) m_IKConstraintEnabled = !m_IKConstraintEnabled;
  if (Levye::Input::IsPressed("IKMinDown"))
    limits.minBend = std::max(0.0f, limits.minBend - bendStep);
  if (Levye::Input::IsPressed("IKMinUp"))
    limits.minBend = std::min(limits.maxBend, limits.minBend + bendStep);
  if (Levye::Input::IsPressed("IKMaxDown"))
    limits.maxBend = std::max(limits.minBend, limits.maxBend - bendStep);
  if (Levye::Input::IsPressed("IKMaxUp")) limits.maxBend = std::min(pi, limits.maxBend + bendStep);
  Vector3& target = m_IKSelectedChain ? m_IK2Target : m_IKTarget;
  Vector3& pole = m_IKSelectedChain ? m_IK2Pole : m_IKPole;
  Vector3& smoothed = m_IKSelectedChain ? m_IK2SmoothedTarget : m_IKSmoothedTarget;
  Vector3* joints = m_IKSelectedChain ? m_IK2JointPositions : m_IKJointPositions;
  auto& selectedConstraint = m_IKSelectedChain ? m_IK2Constraint : m_IKConstraint;
  const float horizontal = static_cast<float>(Levye::Input::IsDown("IKTargetRight")) -
                           static_cast<float>(Levye::Input::IsDown("IKTargetLeft"));
  const float vertical = static_cast<float>(Levye::Input::IsDown("IKTargetUp")) -
                         static_cast<float>(Levye::Input::IsDown("IKTargetDown"));
  const float depth = static_cast<float>(Levye::Input::IsDown("IKTargetBackward")) -
                      static_cast<float>(Levye::Input::IsDown("IKTargetForward"));
  IKSceneLogic::MoveTarget(target, horizontal, vertical, depth, deltaTime);

  m_IKSolved = false;
  if (!Levye::Animation::SamplePose(m_IKPose)) {
    m_IKReadback = false;
    Record(7, false, "Sample IK pose");
    return;
  }
  if (m_IKRoot >= 0 && ReadIKJoints()) {
    if (Levye::Input::IsPressed("IKReset")) {
      if (!IKSceneLogic::ResetTarget({"", -1, joints[0]}, {"", -1, joints[1]}, {"", -1, joints[2]},
                                     target, pole))
        Record(7, false, "Reset IK target");
      else {
        smoothed = target;
        SetPreferredBend(joints, selectedConstraint);
      }
    }
    if (Levye::Input::IsPressed("IKPoleReverse")) {
      const Vector3 root = joints[0];
      pole = {2.0f * root.x - pole.x, 2.0f * root.y - pole.y, 2.0f * root.z - pole.z};
      selectedConstraint.hasPreviousDirection = 0;
      const Vector3 preferred = Vector3Subtract(pole, root);
      selectedConstraint.preferredDirection[0] = preferred.x;
      selectedConstraint.preferredDirection[1] = preferred.y;
      selectedConstraint.preferredDirection[2] = preferred.z;
    }
    m_IKSmoothedTarget =
        Levye::SmoothTarget(m_IKSmoothedTarget, m_IKTarget, deltaTime, m_IKSmoothingResponse);
    if (m_IK2Root >= 0)
      m_IK2SmoothedTarget =
          Levye::SmoothTarget(m_IK2SmoothedTarget, m_IK2Target, deltaTime, m_IKSmoothingResponse);
    Levye::AnimationIKChainRequest requests[2]{};
    const auto configure = [&](int index, int root, int middle, int end, Vector3 t, Vector3 p,
                               Levye::AnimationIKConstraint constraint) {
      auto& request = requests[index];
      request.id = static_cast<std::uint64_t>(index + 1);
      request.root = root;
      request.middle = middle;
      request.end = end;
      request.target[0] = t.x;
      request.target[1] = t.y;
      request.target[2] = t.z;
      request.pole[0] = p.x;
      request.pole[1] = p.y;
      request.pole[2] = p.z;
      request.weight = m_IKWeight;
      request.tolerance = 0.01f;
      request.constraint = constraint;
      request.constrained = m_IKConstraintEnabled ? 1U : 0U;
      request.enabled = m_IKEnabled && root >= 0 ? 1U : 0U;
    };
    configure(0, m_IKRoot, m_IKMiddle, m_IKEnd, m_IKSmoothedTarget, m_IKPole, m_IKConstraint);
    configure(1, m_IK2Root, m_IK2Middle, m_IK2End, m_IK2SmoothedTarget, m_IK2Pole, m_IK2Constraint);
    Levye::AnimationIKChainResult results[2]{};
    if (!Levye::Animation::SolveIKChains(m_IKPose, requests, 2, results))
      Record(7, false, "Solve IK batch");
    m_IKStatus = results[0].status;
    m_IK2Status = results[1].status;
    m_IKSolved = m_IKStatus == Levye::AnimationIKChainStatus::Solved;
    if (m_IKSolved) {
      m_IKResult = results[0].ik;
      m_IKReachable = m_IKResult.reachable != 0;
      for (int i = 0; i < 3; ++i) m_IKConstraint.previousDirection[i] = m_IKResult.bendDirection[i];
      m_IKConstraint.hasPreviousDirection = 1;
    }
    if (m_IK2Status == Levye::AnimationIKChainStatus::Solved) {
      m_IK2Result = results[1].ik;
      for (int i = 0; i < 3; ++i)
        m_IK2Constraint.previousDirection[i] = m_IK2Result.bendDirection[i];
      m_IK2Constraint.hasPreviousDirection = 1;
    }
    if (m_IKEnabled &&
        (!m_IKSolved || (m_IK2Root >= 0 && m_IK2Status != Levye::AnimationIKChainStatus::Solved)))
      Record(7, false, "Solve IK batch");
  }
  if (!Levye::Animation::ApplyPose(m_IKPose, &m_Model)) Record(7, false, "Apply IK pose");
  const bool readback = m_IKRoot >= 0 && ReadIKJoints();
  if (readback) m_IKEndError = IKSceneLogic::Distance(m_IKJointPositions[2], m_IKSmoothedTarget);
  if (readback && m_IK2Root >= 0)
    m_IK2EndError = IKSceneLogic::Distance(m_IK2JointPositions[2], m_IK2SmoothedTarget);
}

bool AnimationScene::SelectScene(int index) {
  if (!SceneReady(index)) return false;
  if (index == 7 && m_IKEndError < 0.0f && m_IKRoot >= 0 &&
      Levye::Animation::SamplePose(m_IKPose) && ReadIKJoints()) {
    if (IKSceneLogic::ResetTarget({"", -1, m_IKJointPositions[0]}, {"", -1, m_IKJointPositions[1]},
                                  {"", -1, m_IKJointPositions[2]}, m_IKTarget, m_IKPole)) {
      m_IKSmoothedTarget = m_IKTarget;
      m_IKEndError = IKSceneLogic::Distance(m_IKJointPositions[2], m_IKSmoothedTarget);
      SetPreferredBend(m_IKJointPositions, m_IKConstraint);
      if (m_IK2Root >= 0 && IKSceneLogic::ResetTarget(
                                {"", -1, m_IK2JointPositions[0]}, {"", -1, m_IK2JointPositions[1]},
                                {"", -1, m_IK2JointPositions[2]}, m_IK2Target, m_IK2Pole)) {
        m_IK2SmoothedTarget = m_IK2Target;
        m_IK2EndError = IKSceneLogic::Distance(m_IK2JointPositions[2], m_IK2Target);
        SetPreferredBend(m_IK2JointPositions, m_IK2Constraint);
      }
    } else {
      Record(7, false, "Initialize current IK target");
    }
  }
  m_CurrentScene = index;
  return true;
}

void AnimationScene::Record(int scene, bool success, const char* operation) {
  auto& view = m_Scenes[static_cast<std::size_t>(scene)];
  view.lastOperationPassed = success;
  std::snprintf(view.diagnostic.data(), view.diagnostic.size(), "%s: %s", operation,
                success ? "PASS" : "REJECTED/FAILED");
}

bool AnimationScene::SelectClip(int scene, int clip) {
  auto& view = m_Scenes[static_cast<std::size_t>(scene)];
  const int index = (clip % m_AnimationCount + m_AnimationCount) % m_AnimationCount;
  view.selectedClip = index;
  const auto handle = m_Animations[static_cast<std::size_t>(index)];
  if (scene == 0) {
    const bool success = Levye::Animation::BindAnimation(view.controller, handle) &&
                         Levye::Animation::PlayBound(view.controller);
    Record(scene, success, "Play selected clip");
    return success;
  }
  return true;
}

bool AnimationScene::SetNamedState(int scene, const char* name, bool force) {
  const auto controller = m_Scenes[static_cast<std::size_t>(scene)].controller;
  const bool success = force ? Levye::Animation::ForceState(controller, name)
                             : Levye::Animation::SetState(controller, name);
  Record(scene, success, force ? "Force state" : "Set state");
  return success;
}

void AnimationScene::UpdateParameters(int scene, float deltaTime) {
  auto& view = m_Scenes[static_cast<std::size_t>(scene)];
  const bool walking = Levye::Input::IsDown("AnimationMove");
  const bool left = scene == 5 && Levye::Input::IsDown("AnimationLeft");
  const bool right = scene == 5 && Levye::Input::IsDown("AnimationRight");
  const bool moving = walking || left || right;
  const float speed = moving ? (Levye::Input::IsDown("AnimationRun") ? 1.0f : 0.5f) : 0.0f;
  const int emote = Levye::Input::IsDown("AnimationDance") ? 1 : 0;
  bool success = Levye::Animation::SetBoolParameter(view.controller, "Moving", moving) &&
                 Levye::Animation::SetFloatParameter(view.controller, "MoveSpeed", speed) &&
                 Levye::Animation::SetIntParameter(view.controller, "Emote", emote);
  if (Levye::Input::IsPressed("AnimationPunch"))
    success = Levye::Animation::SetTrigger(view.controller, "Punch") && success;
  if (Levye::Input::IsPressed("AnimationJump"))
    success = Levye::Animation::SetTrigger(view.controller, "Jump") && success;
  if (!success) Record(scene, false, "Set conditions");
  if (scene == 5) {
    const float direction = static_cast<float>(right) - static_cast<float>(left);
    view.position = std::clamp(view.position + direction * speed * deltaTime * 3.0f, -6.0f, 6.0f);
  }
}

bool AnimationScene::RecreateLifecycleController() {
  auto& view = m_Scenes[6];
  const auto stale = view.controller;
  Levye::Animation::DestroyController(stale);
  const bool invalidated = !Levye::Animation::IsControllerValid(stale);
  view.controller = Levye::INVALID_ANIMATION_CONTROLLER;
  const bool recreated = ConfigureController(6);
  Record(6, invalidated && recreated, "Stale handle invalidation and recreation");
  return invalidated && recreated;
}

void AnimationScene::Update(float deltaTime) {
  if (!SceneReady(m_CurrentScene)) return;
  const int scene = m_CurrentScene;
  auto& view = m_Scenes[static_cast<std::size_t>(scene)];
  const auto controller = view.controller;

  if (scene == 0) {
    if (Levye::Input::IsPressed("AnimationPrevious"))
      (void)SelectClip(scene, view.selectedClip - 1);
    if (Levye::Input::IsPressed("AnimationNext")) (void)SelectClip(scene, view.selectedClip + 1);
    if (Levye::Input::IsPressed("AnimationRestart")) (void)SelectClip(scene, view.selectedClip);
    const int boneCount = Levye::Animation::GetPoseBoneCount(m_EditablePose);
    if (boneCount > 0 && Levye::Input::IsPressed("AnimationBoneNext")) {
      m_SelectedBone = (m_SelectedBone + 1) % boneCount;
      m_BoneAngle = 0.0f;
    }
    if (boneCount > 0 && Levye::Input::IsPressed("AnimationBonePrevious")) {
      m_SelectedBone = (m_SelectedBone + boneCount - 1) % boneCount;
      m_BoneAngle = 0.0f;
    }
    if (Levye::Input::IsDown("AnimationBonePositive")) m_BoneAngle += deltaTime;
    if (Levye::Input::IsDown("AnimationBoneNegative")) m_BoneAngle -= deltaTime;
    if (Levye::Input::IsPressed("AnimationBoneReset")) m_BoneAngle = 0.0f;
  } else if (scene == 1) {
    if (Levye::Input::IsPressed("AnimationPrevious"))
      (void)SelectClip(scene, view.selectedClip - 1);
    if (Levye::Input::IsPressed("AnimationNext")) (void)SelectClip(scene, view.selectedClip + 1);
    if (Levye::Input::IsPressed("AnimationFaster"))
      view.crossfadeDuration = std::min(view.crossfadeDuration + 0.25f, 3.0f);
    if (Levye::Input::IsPressed("AnimationSlower"))
      view.crossfadeDuration = std::max(view.crossfadeDuration - 0.25f, 0.0f);
    if (Levye::Input::IsPressed("AnimationConfirm"))
      Record(scene,
             Levye::Animation::CrossFade(controller,
                                         m_Animations[static_cast<std::size_t>(view.selectedClip)],
                                         view.crossfadeDuration),
             "Crossfade to selected clip");
    if (Levye::Input::IsPressed("AnimationPunch")) {
      (void)SelectClip(scene, view.selectedClip + 1);
      Record(scene,
             Levye::Animation::CrossFade(controller,
                                         m_Animations[static_cast<std::size_t>(view.selectedClip)],
                                         view.crossfadeDuration),
             "Interrupt crossfade");
    }
    if (Levye::Input::IsPressed("AnimationRestart")) {
      const int idle = FindClip("Robot_Idle");
      Record(scene,
             idle >= 0 &&
                 Levye::Animation::BindAnimation(controller,
                                                 m_Animations[static_cast<std::size_t>(idle)]) &&
                 Levye::Animation::PlayBound(controller),
             "Reset to idle");
    }
  } else if (scene == 2) {
    if (Levye::Input::IsPressed("AnimationPrevious"))
      (void)SelectClip(scene, view.selectedClip - 1);
    if (Levye::Input::IsPressed("AnimationNext")) (void)SelectClip(scene, view.selectedClip + 1);
    if (Levye::Input::IsPressed("AnimationConfirm"))
      (void)SetNamedState(scene, m_Names[static_cast<std::size_t>(view.selectedClip)].data(),
                          false);
    if (Levye::Input::IsPressed("AnimationForce"))
      (void)SetNamedState(scene, m_Names[static_cast<std::size_t>(view.selectedClip)].data(), true);
    if (Levye::Input::IsPressed("AnimationRestart")) {
      char name[48]{};
      const bool success = Levye::Animation::GetCurrentState(controller, name, sizeof(name)) &&
                           name[0] && Levye::Animation::RestartState(controller, name);
      Record(scene, success, "Restart current state");
    }
  } else if (scene == 3 || scene == 5) {
    UpdateParameters(scene, deltaTime);
    if (Levye::Input::IsPressed("AnimationForce")) (void)SetNamedState(scene, "Robot_Idle", true);
    if (Levye::Input::IsPressed("AnimationRestart")) {
      char name[48]{};
      const bool success = Levye::Animation::GetCurrentState(controller, name, sizeof(name)) &&
                           name[0] && Levye::Animation::RestartState(controller, name);
      Record(scene, success, "Restart current state");
    }
  } else if (scene == 4) {
    if (Levye::Input::IsPressed("AnimationJump")) (void)SetNamedState(scene, "Robot_Jump", false);
    if (Levye::Input::IsPressed("AnimationPunch")) (void)SetNamedState(scene, "Robot_Punch", false);
    if (Levye::Input::IsPressed("AnimationDeath")) (void)SetNamedState(scene, "Robot_Death", false);
    if (Levye::Input::IsPressed("AnimationWave")) (void)SetNamedState(scene, "Robot_Wave", false);
    if (Levye::Input::IsPressed("AnimationConfirm"))
      (void)SetNamedState(scene, "Robot_Idle", false);
    if (Levye::Input::IsPressed("AnimationForce")) (void)SetNamedState(scene, "Robot_Idle", true);
    if (Levye::Input::IsPressed("AnimationRestart")) {
      char name[48]{};
      const bool success = Levye::Animation::GetCurrentState(controller, name, sizeof(name)) &&
                           name[0] && Levye::Animation::RestartState(controller, name);
      Record(scene, success, "Restart one-shot");
    }
  } else if (scene == 6) {
    if (Levye::Input::IsPressed("AnimationRestart")) (void)RecreateLifecycleController();
    if (Levye::Input::IsPressed("AnimationUnload")) {
      const int count = Levye::Animation::GetAnimationCount(m_ModelPath.data());
      std::vector<Levye::AnimationResourceHandle> temporary(
          static_cast<std::size_t>(std::max(count, 0)));
      bool success = count > 0 && Levye::Animation::LoadAnimations(
                                      m_ModelPath.data(), temporary.data(), count) == count;
      if (success) {
        const auto stale = temporary.front();
        for (auto handle : temporary)
          success = Levye::Animation::UnloadAnimation(handle) && success;
        char name[48]{};
        success = !Levye::Animation::GetAnimationName(stale, name, sizeof(name)) && success;
        success = Levye::Animation::IsControllerValid(controller) && success;
      }
      Record(scene, success, "Temporary resource unload");
    }
    if (Levye::Input::IsPressed("AnimationVerify"))
      Record(scene, VerifyPersistentHandles(), "Persistent handle verification");
  } else if (scene == 7) {
    UpdateIK(deltaTime);
    return;
  } else if (scene == 8) {
    UpdateContacts(deltaTime);
    return;
  } else if (scene == 9) {
    m_Locomotion.Update(deltaTime);
    return;
  } else if (scene == 10) {
    m_Orientation.Update(deltaTime);
    return;
  }

  if (Levye::Input::IsPressed("AnimationPause")) {
    Levye::AnimationPlaybackState playback{};
    const bool valid = Levye::Animation::GetPlaybackState(view.controller, playback);
    const bool success = valid && (playback.paused ? Levye::Animation::Resume(view.controller)
                                                   : Levye::Animation::Pause(view.controller));
    Record(scene, success, playback.paused ? "Resume" : "Pause");
  }
  if (scene == 0) {
    Levye::AnimationPlaybackState playback{};
    if (Levye::Animation::GetPlaybackState(view.controller, playback)) {
      if (Levye::Input::IsPressed("AnimationFaster"))
        Record(scene,
               Levye::Animation::SetSpeed(view.controller, std::min(playback.speed + 0.25f, 4.0f)),
               "Increase speed");
      if (Levye::Input::IsPressed("AnimationSlower"))
        Record(scene,
               Levye::Animation::SetSpeed(view.controller, std::max(playback.speed - 0.25f, 0.0f)),
               "Decrease speed");
    }
  }
  if (scene == 0) {
    Levye::AnimationBoneTransform bone{};
    bool success = Levye::Animation::SamplePose(m_EditablePose) &&
                   Levye::Animation::GetPoseBoneTransform(m_EditablePose, m_SelectedBone,
                                                          Levye::AnimationBoneSpace::Local, bone);
    if (success) {
      const Quaternion base{bone.rotation[0], bone.rotation[1], bone.rotation[2], bone.rotation[3]};
      const Quaternion twist = QuaternionFromAxisAngle({0.0f, 1.0f, 0.0f}, m_BoneAngle);
      const Quaternion result = QuaternionMultiply(base, twist);
      bone.rotation[0] = result.x;
      bone.rotation[1] = result.y;
      bone.rotation[2] = result.z;
      bone.rotation[3] = result.w;
      success = Levye::Animation::SetPoseBoneTransform(m_EditablePose, m_SelectedBone,
                                                       Levye::AnimationBoneSpace::Local, bone) &&
                Levye::Animation::ApplyPose(m_EditablePose, &m_Model);
    }
    if (!success) Record(scene, false, "Apply edited pose");
  } else if (!Levye::Animation::ApplyBoundAnimation(view.controller, &m_Model)) {
    Record(scene, false, "Apply pose");
  }
}

Camera3D AnimationScene::IKCamera() const {
  if (!m_IKReadback) return m_Camera;
  std::array<Vector3, 12> points{IKSceneLogic::ToWorld(m_IKJointPositions[0], m_Model.transform),
                                 IKSceneLogic::ToWorld(m_IKJointPositions[1], m_Model.transform),
                                 IKSceneLogic::ToWorld(m_IKJointPositions[2], m_Model.transform),
                                 IKSceneLogic::ToWorld(m_IKTarget, m_Model.transform),
                                 IKSceneLogic::ToWorld(m_IKSmoothedTarget, m_Model.transform),
                                 IKSceneLogic::ToWorld(m_IKPole, m_Model.transform)};
  int pointCount = 6;
  if (m_IK2Root >= 0) {
    for (int i = 0; i < 3; ++i)
      points[static_cast<std::size_t>(pointCount++)] =
          IKSceneLogic::ToWorld(m_IK2JointPositions[i], m_Model.transform);
    points[static_cast<std::size_t>(pointCount++)] =
        IKSceneLogic::ToWorld(m_IK2Target, m_Model.transform);
    points[static_cast<std::size_t>(pointCount++)] =
        IKSceneLogic::ToWorld(m_IK2SmoothedTarget, m_Model.transform);
    points[static_cast<std::size_t>(pointCount++)] =
        IKSceneLogic::ToWorld(m_IK2Pole, m_Model.transform);
  }
  Vector3 minimum = points[0];
  Vector3 maximum = points[0];
  for (int i = 0; i < pointCount; ++i) {
    const Vector3 point = points[static_cast<std::size_t>(i)];
    minimum = Vector3Min(minimum, point);
    maximum = Vector3Max(maximum, point);
  }
  const Vector3 center = Vector3Scale(Vector3Add(minimum, maximum), 0.5f);
  float radius = 0.0f;
  for (int i = 0; i < pointCount; ++i)
    radius = std::max(radius, Vector3Distance(center, points[static_cast<std::size_t>(i)]));
  radius = std::max(radius, 0.8f);

  // Aim to the camera's right of the chain, leaving the robot left of the HUD.
  const Vector3 view = Vector3Normalize({3.0f, 2.0f, 5.0f});
  const Vector3 right =
      Vector3Normalize(Vector3CrossProduct(Vector3Negate(view), {0.0f, 1.0f, 0.0f}));
  Camera3D camera{};
  camera.target = Vector3Add(center, Vector3Scale(right, radius * 0.65f));
  camera.position = Vector3Add(camera.target, Vector3Scale(view, std::max(4.0f, radius * 3.3f)));
  camera.up = {0.0f, 1.0f, 0.0f};
  camera.fovy = 40.0f;
  camera.projection = CAMERA_PERSPECTIVE;
  return camera;
}

void AnimationScene::DrawIKHUD() const {
  const int selectedRoot = m_IKSelectedChain ? m_IK2Root : m_IKRoot;
  const int selectedMiddle = m_IKSelectedChain ? m_IK2Middle : m_IKMiddle;
  const int selectedEnd = m_IKSelectedChain ? m_IK2End : m_IKEnd;
  const Vector3 selectedTarget = m_IKSelectedChain ? m_IK2Target : m_IKTarget;
  const Vector3 selectedPole = m_IKSelectedChain ? m_IK2Pole : m_IKPole;
  const Vector3 selectedSmoothed = m_IKSelectedChain ? m_IK2SmoothedTarget : m_IKSmoothedTarget;
  const auto& selectedConstraint = m_IKSelectedChain ? m_IK2Constraint : m_IKConstraint;
  const auto& selectedResult = m_IKSelectedChain ? m_IK2Result : m_IKResult;
  const bool selectedSolved =
      (m_IKSelectedChain ? m_IK2Status : m_IKStatus) == Levye::AnimationIKChainStatus::Solved;
  const bool selectedReachable =
      selectedSolved && (m_IKSelectedChain ? m_IK2Result.reachable != 0 : m_IKReachable);
  const float selectedError = m_IKSelectedChain ? m_IK2EndError : m_IKEndError;
  const int width = 360;
  const int x = std::max(10, GetScreenWidth() - width - 12);
  const int y = 12;
  DrawRectangle(x, y, width, 518, Fade(BLACK, 0.82f));
  DrawText(TextFormat("8/8 IK  Arm %d", m_IKSelectedChain + 1), x + 12, y + 10, 21, RAYWHITE);
  DrawText("F1-F11 scenes  |  T next  |  ESC menu", x + 12, y + 38, 15, YELLOW);

  char rootName[32]{};
  char middleName[32]{};
  char endName[32]{};
  const bool chain =
      m_IKReadback &&
      Levye::Animation::GetPoseBoneName(m_IKPose, selectedRoot, rootName, sizeof(rootName)) &&
      Levye::Animation::GetPoseBoneName(m_IKPose, selectedMiddle, middleName, sizeof(middleName)) &&
      Levye::Animation::GetPoseBoneName(m_IKPose, selectedEnd, endName, sizeof(endName));
  const char* status = !chain           ? "NO CHAIN"
                       : !m_IKEnabled   ? "OFF"
                       : selectedSolved ? "SUCCESS"
                                        : "FAILED";
  DrawText(TextFormat("IK: %s    Solver: %s", m_IKEnabled ? "ON" : "OFF", status), x + 12, y + 67,
           16, selectedSolved || !m_IKEnabled ? GREEN : ORANGE);
  DrawText(chain ? (!selectedSolved     ? "Target: not evaluated"
                    : selectedReachable ? "Target: REACHABLE"
                                        : "Target: UNREACHABLE (clamped)")
                 : "Target: unavailable",
           x + 12, y + 90, 16, !chain || selectedReachable ? LIGHTGRAY : ORANGE);
  DrawText(
      chain ? TextFormat("End to smoothed: %.3f", selectedError) : m_Scenes[7].diagnostic.data(),
      x + 12, y + 113, 16, WHITE);
  DrawText(chain ? TextFormat("Root: %s [%d]", rootName, selectedRoot) : "Root: -", x + 12, y + 140,
           15, RED);
  DrawText(chain ? TextFormat("Middle: %s [%d]", middleName, selectedMiddle) : "Middle: -", x + 12,
           y + 160, 15, ORANGE);
  DrawText(chain ? TextFormat("End: %s [%d]", endName, selectedEnd) : "End: -", x + 12, y + 180, 15,
           YELLOW);
  DrawText(
      TextFormat("Desired: %.2f %.2f %.2f", selectedTarget.x, selectedTarget.y, selectedTarget.z),
      x + 12, y + 207, 15, SKYBLUE);
  DrawText(TextFormat("Smoothed: %.2f %.2f %.2f", selectedSmoothed.x, selectedSmoothed.y,
                      selectedSmoothed.z),
           x + 12, y + 227, 15, GOLD);
  DrawText(TextFormat("Pole: %.2f %.2f %.2f", selectedPole.x, selectedPole.y, selectedPole.z),
           x + 12, y + 247, 15, VIOLET);
  constexpr float degrees = 57.2957795f;
  DrawText(TextFormat("Limits: %s  %.0f..%.0f deg", m_IKConstraintEnabled ? "ON" : "OFF",
                      selectedConstraint.minBend * degrees, selectedConstraint.maxBend * degrees),
           x + 12, y + 274, 15, m_IKConstraintEnabled ? MAGENTA : LIGHTGRAY);
  DrawText(TextFormat("Solved bend: %.1f deg", selectedResult.bendAngle * degrees), x + 12, y + 294,
           15, WHITE);
  DrawText(TextFormat("Pole: %s", selectedResult.usedPreviousDirection   ? "previous bend"
                                  : selectedResult.usedFallbackDirection ? "axis fallback"
                                                                         : "preferred / pole"),
           x + 12, y + 314, 15, VIOLET);
  DrawText(TextFormat("Weight: %.1f   Response: %.0f/s", m_IKWeight, m_IKSmoothingResponse), x + 12,
           y + 340, 15, WHITE);
  DrawText("1 limits  2/3 min  4/5 max", x + 12, y + 366, 15, WHITE);
  DrawText("[ / ] weight   - / = response", x + 12, y + 387, 15, WHITE);
  DrawText("I toggle  J/L left/right  U/O depth", x + 12, y + 408, 15, WHITE);
  DrawText("K/M up/down  P pole  R reset", x + 12, y + 429, 15, WHITE);
  DrawText("TAB select arm", x + 12, y + 450, 15, YELLOW);
  const auto statusName = [](Levye::AnimationIKChainStatus status) {
    using Status = Levye::AnimationIKChainStatus;
    switch (status) {
      case Status::Solved:
        return "SOLVED";
      case Status::Disabled:
        return "OFF";
      case Status::Conflict:
        return "CONFLICT";
      case Status::Invalid:
        return "INVALID";
      case Status::Failed:
        return "FAILED";
    }
    return "UNKNOWN";
  };
  DrawText(TextFormat("Arm 1: %s  err %.3f", statusName(m_IKStatus), m_IKEndError), x + 12, y + 472,
           15, SKYBLUE);
  DrawText(TextFormat("Arm 2: %s  err %.3f", statusName(m_IK2Status), m_IK2EndError), x + 12,
           y + 492, 15, LIME);
}

void AnimationScene::DrawContacts() const {
  if (!m_ContactInitialized) return;
  for (int i = 0; i < 2; ++i) {
    const Vector3 origin = m_ContactOrigins[i];
    const Vector3 flat = ContactSurfacePoint(origin, 0);
    const Vector3 raised = ContactSurfacePoint(origin, 1);
    const Vector3 slope = ContactSurfacePoint(origin, 2);
    DrawCube(flat, 0.45f, 0.04f, 0.45f, m_ContactSurface == 0 ? SKYBLUE : GRAY);
    DrawCube(raised, 0.45f, 0.12f, 0.45f, m_ContactSurface == 1 ? GOLD : DARKGRAY);
    const Vector3 a = Vector3Add(slope, {-0.35f, -0.16f, -0.35f});
    const Vector3 b = Vector3Add(slope, {0.35f, 0.16f, -0.35f});
    const Vector3 c = Vector3Add(slope, {0.35f, 0.16f, 0.35f});
    const Vector3 d = Vector3Add(slope, {-0.35f, -0.16f, 0.35f});
    DrawTriangle3D(a, c, b, m_ContactSurface == 2 ? LIME : DARKGREEN);
    DrawTriangle3D(a, d, c, m_ContactSurface == 2 ? LIME : DARKGREEN);
    const auto& contact = m_Contacts[i];
    const Color color = i == 0 ? SKYBLUE : ORANGE;
    DrawSphereWires(contact.targetWorld, 0.12f, 10, 12, color);
    DrawSphere(m_ContactEndWorld[i], 0.08f, color);
    DrawLine3D(m_ContactEndWorld[i], contact.targetWorld, color);
    DrawLine3D(contact.targetWorld,
               Vector3Add(contact.targetWorld, Vector3Scale(contact.normalWorld, 0.35f)), GREEN);
    if (i == m_ContactSelected) DrawSphereWires(contact.targetWorld, 0.18f, 10, 12, WHITE);
  }
}

void AnimationScene::Draw() const {
  if (!SceneReady(m_CurrentScene)) return;
  if (m_CurrentScene == 9) {
    m_Locomotion.Draw();
    return;
  }
  if (m_CurrentScene == 10) {
    m_Orientation.Draw();
    return;
  }
  const auto& view = m_Scenes[static_cast<std::size_t>(m_CurrentScene)];
  const auto controller = view.controller;
  const float x = m_CurrentScene == 5   ? view.position
                  : m_CurrentScene == 8 ? m_ContactRootPosition.x
                                        : 0.0f;
  BeginMode3D(m_CurrentScene == 7 ? IKCamera() : m_Camera);
  DrawGrid(20, 1.0f);
  DrawModel(m_Model, {x, 0.0f, 0.0f}, 1.0f, WHITE);
  if (m_CurrentScene == 8) DrawContacts();
  if (m_CurrentScene == 7) {
    const Vector3 target = IKSceneLogic::ToWorld(m_IKTarget, m_Model.transform);
    const Vector3 smoothed = IKSceneLogic::ToWorld(m_IKSmoothedTarget, m_Model.transform);
    const Vector3 pole = IKSceneLogic::ToWorld(m_IKPole, m_Model.transform);
    DrawSphereWires(target, 0.17f, 10, 14, SKYBLUE);
    if (m_IKSelectedChain == 0) DrawSphereWires(target, 0.24f, 10, 14, WHITE);
    DrawSphere(smoothed, 0.11f, GOLD);
    DrawLine3D(target, smoothed, SKYBLUE);
    DrawCube(pole, 0.22f, 0.22f, 0.22f, VIOLET);
    DrawCubeWires(pole, 0.25f, 0.25f, 0.25f, WHITE);
    if (m_IKReadback) {
      const Vector3 root = IKSceneLogic::ToWorld(m_IKJointPositions[0], m_Model.transform);
      const Vector3 middle = IKSceneLogic::ToWorld(m_IKJointPositions[1], m_Model.transform);
      const Vector3 end = IKSceneLogic::ToWorld(m_IKJointPositions[2], m_Model.transform);
      DrawCylinderEx(root, middle, 0.035f, 0.035f, 8, ORANGE);
      DrawCylinderEx(middle, end, 0.035f, 0.035f, 8, YELLOW);
      DrawLine3D(end, smoothed, m_IKConstraintEnabled ? MAGENTA : GOLD);
      const Vector3 preferredModel = Vector3Add(
          m_IKJointPositions[0],
          Vector3Scale({m_IKConstraint.preferredDirection[0], m_IKConstraint.preferredDirection[1],
                        m_IKConstraint.preferredDirection[2]},
                       0.7f));
      const Vector3 preferred = IKSceneLogic::ToWorld(preferredModel, m_Model.transform);
      DrawLine3D(root, preferred, MAGENTA);
      DrawSphere(preferred, 0.06f, MAGENTA);
      DrawSphere(root, 0.11f, RED);
      DrawSphere(middle, 0.11f, ORANGE);
      DrawSphere(end, 0.11f, YELLOW);
    }
    if (m_IK2Root >= 0 && m_IKReadback) {
      const Vector3 secondTarget = IKSceneLogic::ToWorld(m_IK2Target, m_Model.transform);
      const Vector3 secondSmoothed = IKSceneLogic::ToWorld(m_IK2SmoothedTarget, m_Model.transform);
      const Vector3 secondPole = IKSceneLogic::ToWorld(m_IK2Pole, m_Model.transform);
      const Vector3 root = IKSceneLogic::ToWorld(m_IK2JointPositions[0], m_Model.transform);
      const Vector3 middle = IKSceneLogic::ToWorld(m_IK2JointPositions[1], m_Model.transform);
      const Vector3 end = IKSceneLogic::ToWorld(m_IK2JointPositions[2], m_Model.transform);
      DrawSphereWires(secondTarget, 0.17f, 10, 14, LIME);
      if (m_IKSelectedChain == 1) DrawSphereWires(secondTarget, 0.24f, 10, 14, WHITE);
      DrawSphere(secondSmoothed, 0.11f, GREEN);
      DrawCube(secondPole, 0.20f, 0.20f, 0.20f, DARKGREEN);
      DrawCylinderEx(root, middle, 0.035f, 0.035f, 8, DARKGREEN);
      DrawCylinderEx(middle, end, 0.035f, 0.035f, 8, LIME);
      DrawLine3D(end, secondSmoothed, LIME);
      DrawSphere(root, 0.10f, DARKGREEN);
      DrawSphere(middle, 0.10f, GREEN);
      DrawSphere(end, 0.10f, LIME);
    }
  }
  EndMode3D();
  if (m_CurrentScene == 8) {
    DrawRectangle(12, 12, 590, 176, Fade(BLACK, 0.78f));
    DrawText("F9  Procedural Contacts", 24, 20, 21, WHITE);
    DrawText("TAB select | C toggle | SPACE release | R reset", 24, 49, 16, LIGHTGRAY);
    DrawText("LEFT/RIGHT surface | A/D move robot root", 24, 69, 16, LIGHTGRAY);
    DrawText(TextFormat("Surface: %s  Selected: %d  Queries: %s",
                        m_ContactSurface == 0   ? "flat"
                        : m_ContactSurface == 1 ? "platform"
                                                : "slope",
                        m_ContactSelected + 1, m_ContactsEnabled ? "ON" : "OFF"),
             24, 91, 16, WHITE);
    for (int i = 0; i < 2; ++i)
      DrawText(
          TextFormat("Arm %d: %s | %s | IK %s | w %.2f | err %.3f", i + 1,
                     ContactStateName(m_Contacts[i].state), ContactHitName(m_Contacts[i].hitStatus),
                     ContactIKName(m_ContactResults[i].status), m_Contacts[i].weight,
                     m_ContactErrors[i]),
          24, 114 + 22 * i, 15, i == 0 ? SKYBLUE : ORANGE);
    return;
  }
  if (m_CurrentScene == 7) {
    DrawIKHUD();
    return;
  }

  char stateName[48]{};
  const bool named = Levye::Animation::GetCurrentState(controller, stateName, sizeof(stateName));
  Levye::AnimationPlaybackState playback{};
  const bool clock = Levye::Animation::GetPlaybackState(controller, playback);
  Levye::AnimationTransitionState transition{};
  const bool transitionValid = Levye::Animation::GetTransitionState(controller, transition);
  const auto bound = Levye::Animation::GetBoundAnimation(controller);
  const char* activeName = named && stateName[0] ? stateName : ClipName(bound);

  DrawRectangle(10, 10, 930, 360, Fade(BLACK, 0.78f));
  DrawText(TextFormat("%d/%d  %s", m_CurrentScene + 1, SceneCount, SceneName(m_CurrentScene)), 20,
           18, 26, RAYWHITE);
  DrawText("F1-F11: scenes | T: next scene | ESC: menu", 20, 54, 18, YELLOW);
  DrawText(TextFormat("State/clip: %s | Selected: %s", activeName,
                      m_Names[static_cast<std::size_t>(view.selectedClip)].data()),
           20, 84, 18, WHITE);
  DrawText(TextFormat("Controller: %s | Bound: %s | API: v%u",
                      Levye::Animation::IsControllerValid(controller) ? "valid" : "INVALID",
                      ClipName(bound), Levye::ANIMATION_API_VERSION),
           20, 110, 17, LIGHTGRAY);
  if (clock) {
    DrawText(TextFormat("Clock: %.2f / %.2f s | Speed: %.2fx | %s", playback.time,
                        playback.duration, playback.speed,
                        playback.paused    ? "PAUSED"
                        : playback.playing ? "PLAYING"
                                           : "FINISHED"),
             20, 136, 17, LIGHTGRAY);
  }
  if (transitionValid && transition.active) {
    const float progress =
        transition.duration > 0.0f ? 100.0f * transition.elapsed / transition.duration : 100.0f;
    DrawText(TextFormat("Crossfade: %s -> %s  %.0f%%", ClipName(transition.source),
                        ClipName(transition.target), progress),
             20, 162, 17, GREEN);
  } else {
    DrawText("Crossfade: inactive", 20, 162, 17, LIGHTGRAY);
  }
  DrawText(TextFormat("Last check: %s", view.diagnostic.data()), 20, 190, 17,
           view.lastOperationPassed ? GREEN : ORANGE);

  const char* controls = "";
  const char* detail = "";
  switch (m_CurrentScene) {
    case 0:
      controls = "LEFT/RIGHT: play clip | UP/DOWN: speed | P: pause | R: restart";
      detail = "[/]: bone | Q/E: rotate bone | B: reset rotation";
      break;
    case 1:
      controls = "LEFT/RIGHT: target | UP/DOWN: fade duration | ENTER: fade | SPACE: interrupt";
      detail = "P: pause | R: idle | Expected: blended pose and progress; interrupt stays smooth.";
      break;
    case 2:
      controls = "LEFT/RIGHT: target | ENTER: set | F: force | R: restart | P: pause";
      detail = "Expected: lower priority is rejected; force overrides; one-shots return.";
      break;
    case 3:
      controls = "W: walk | SHIFT+W: run | C: dance | SPACE: punch | J: jump";
      detail = "F: force idle | R: restart | Expected: typed conditions select states.";
      break;
    case 4:
      controls = "J: jump | SPACE: punch | D: death | V: wave | ENTER: request idle";
      detail = "F: force idle | R: restart | Expected: one-shots finish then return to idle.";
      break;
    case 5:
      controls = "A/D: move robot | W: walk | SHIFT: run | C: dance";
      detail = "SPACE: punch | J: jump | F: force idle | Expected: pose and position respond.";
      break;
    case 6:
      controls = "R: recreate controller | U: load/unload temporary clips | V: verify handles";
      detail = "Rebuild Game while running; reload count rises and state/resources persist.";
      break;
  }
  DrawText(controls, 20, 227, 17, YELLOW);
  DrawText(detail, 20, 252, 16, LIGHTGRAY);
  if (m_CurrentScene == 0) {
    char boneName[32]{};
    Levye::AnimationBoneTransform local{};
    Levye::AnimationBoneTransform model{};
    const bool valid =
        Levye::Animation::GetPoseBoneName(m_EditablePose, m_SelectedBone, boneName,
                                          sizeof(boneName)) &&
        Levye::Animation::GetPoseBoneTransform(m_EditablePose, m_SelectedBone,
                                               Levye::AnimationBoneSpace::Local, local) &&
        Levye::Animation::GetPoseBoneTransform(m_EditablePose, m_SelectedBone,
                                               Levye::AnimationBoneSpace::Model, model);
    DrawText(
        valid ? TextFormat("Bone %d/%d: %s | twist %.1f deg | model (%.2f, %.2f, %.2f)",
                           m_SelectedBone + 1, Levye::Animation::GetPoseBoneCount(m_EditablePose),
                           boneName, m_BoneAngle * RAD2DEG, model.translation[0],
                           model.translation[1], model.translation[2])
              : "Editable pose unavailable",
        20, 280, 17, valid ? GREEN : ORANGE);
  }
  if (m_CurrentScene == 1)
    DrawText(TextFormat("Fade duration: %.2f s", view.crossfadeDuration), 20, 280, 17, WHITE);
  if (m_CurrentScene == 2 || m_CurrentScene == 4)
    DrawText(TextFormat("Current state priority: %d", StatePriority(activeName)), 20, 280, 17,
             WHITE);
  if (m_CurrentScene == 3 || m_CurrentScene == 5) {
    const bool moving = Levye::Input::IsDown("AnimationMove") ||
                        (m_CurrentScene == 5 && (Levye::Input::IsDown("AnimationLeft") ||
                                                 Levye::Input::IsDown("AnimationRight")));
    const float speed = moving ? (Levye::Input::IsDown("AnimationRun") ? 1.0f : 0.5f) : 0.0f;
    DrawText(
        TextFormat("Conditions: Moving=%s  MoveSpeed=%.1f  Emote=%d", moving ? "true" : "false",
                   speed, Levye::Input::IsDown("AnimationDance") ? 1 : 0),
        20, m_CurrentScene == 3 ? 280 : 307, 17, WHITE);
  }
  if (m_CurrentScene == 5)
    DrawText(TextFormat("Robot X position: %.2f", view.position), 20, 280, 17, WHITE);
  if (m_CurrentScene == 6)
    DrawText(TextFormat("Hot reloads observed: %u | Persistence: %s", m_ReloadCount,
                        m_ReloadPassed ? "PASS" : "FAIL"),
             20, 280, 17, m_ReloadPassed ? GREEN : RED);
  if (Levye::Animation::IsStateFinished(controller))
    DrawText("One-shot final frame reached", 20, 333, 17, GREEN);
}

bool AnimationScene::VerifyPersistentHandles() const {
  bool valid = m_Ready && IsModelValid(m_Model);
  valid = Levye::Animation::IsPoseValid(m_EditablePose) &&
          Levye::Animation::GetPoseBoneCount(m_EditablePose) > 0 && valid;
  for (int scene = 0; scene < SceneCount; ++scene) {
    const auto controller = m_Scenes[static_cast<std::size_t>(scene)].controller;
    char name[48]{};
    Levye::AnimationPlaybackState playback{};
    valid = Levye::Animation::IsControllerValid(controller) &&
            Levye::Animation::GetCurrentState(controller, name, sizeof(name)) &&
            Levye::Animation::GetPlaybackState(controller, playback) && valid;
  }
  for (int i = 0; i < m_AnimationCount; ++i) {
    char name[48]{};
    valid = Levye::Animation::GetAnimationName(m_Animations[static_cast<std::size_t>(i)], name,
                                               sizeof(name)) &&
            valid;
  }

  valid = Levye::Animation::IsPoseValid(m_IKPose) && valid;
  valid = Levye::Animation::IsPoseValid(m_ContactPose) && m_Locomotion.IsReady() &&
          m_Orientation.IsReady() && valid;
  return valid;
}

bool AnimationScene::AfterReload() {
  ++m_ReloadCount;
  m_ReloadPassed = VerifyPersistentHandles();
  return m_ReloadPassed;
}

void AnimationScene::Shutdown() {
  if (m_EditablePose != Levye::INVALID_ANIMATION_POSE)
    Levye::Animation::DestroyPose(m_EditablePose);
  m_EditablePose = Levye::INVALID_ANIMATION_POSE;
  if (m_IKPose != Levye::INVALID_ANIMATION_POSE) Levye::Animation::DestroyPose(m_IKPose);
  m_IKPose = Levye::INVALID_ANIMATION_POSE;
  if (m_ContactPose != Levye::INVALID_ANIMATION_POSE) Levye::Animation::DestroyPose(m_ContactPose);
  m_ContactPose = Levye::INVALID_ANIMATION_POSE;
  m_Locomotion = {};
  m_Orientation = {};
  for (auto& view : m_Scenes) {
    if (view.controller != Levye::INVALID_ANIMATION_CONTROLLER)
      Levye::Animation::DestroyController(view.controller);
    view = {};
  }
  for (int i = 0; i < m_AnimationCount; ++i)
    (void)Levye::Animation::UnloadAnimation(m_Animations[static_cast<std::size_t>(i)]);
  if (IsModelValid(m_Model)) UnloadModel(m_Model);
  m_Model = {};
  m_Animations.fill(Levye::INVALID_ANIMATION_RESOURCE);
  m_AnimationCount = 0;
  m_CurrentScene = 0;
  m_ReloadCount = 0;
  m_ReloadPassed = true;
  m_ModelPath.fill('\0');
  m_Ready = false;
  m_SelectedBone = 0;
  m_BoneAngle = 0.0f;

  m_IKRoot = -1;
  m_IKMiddle = -1;
  m_IKEnd = -1;
  m_IKEnabled = true;
  m_IKSolved = false;
  m_IKReadback = false;
  m_IKReachable = false;
  m_IKEndError = -1.0f;
  m_IKTarget = {0.5f, 1.5f, 0.0f};
  m_IKPole = {0.0f, 1.5f, 1.0f};
  m_IK2Root = m_IK2Middle = m_IK2End = -1;
  m_IKSelectedChain = 0;
  m_IK2Target = {};
  m_IK2Pole = {};
  m_IK2SmoothedTarget = {};
  m_IK2EndError = -1.0f;
  m_IKStatus = Levye::AnimationIKChainStatus::Disabled;
  m_IK2Status = Levye::AnimationIKChainStatus::Disabled;
  m_IKConstraint.hasPreviousDirection = 0;
  m_IK2Constraint.hasPreviousDirection = 0;
}

}  // namespace Sandbox
