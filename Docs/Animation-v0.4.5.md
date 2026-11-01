# Animation module guide (API v15)

Enable the optional module with `-DLEVYE_WITH_ANIMATION=ON`, or create a project with `levye new MyGame --with animation`. Include `<Levye/Modules/Animation/Animation.hpp>` in a game module. The module must have a bound `HostServices` table; `Animation::IsAvailable()` returns false when it is absent or incompatible.

For the basic application lifecycle, start with [`Templates/Default/Source/Main.cpp`](../Templates/Default/Source/Main.cpp) and [`Templates/Default/Source/Game.cpp`](../Templates/Default/Source/Game.cpp): the host loads and starts the module, then runs the application; game state is constructed at initial load and destroyed at final shutdown.

The host owns clips, controllers, and editable poses through generation-safe handles. Create and load them once during normal game initialization, keep their handles in persistent game state across compatible hot reloads, and destroy them during normal shutdown. The game owns the raylib `Model`. Calls that accept `Model*` borrow it only for that call. A clip and model must have the same bone ordering.

## Playback and pose

```cpp
// Resolve this path using your project's asset directory before loading.
const char* clipPath = "Examples/Sandbox/Assets/robot.glb";
const int count = Levye::Animation::GetAnimationCount(clipPath);
std::vector<Levye::AnimationResourceHandle> clips(count);
if (count > 0 && Levye::Animation::LoadAnimations(clipPath, clips.data(), count) == count) {
    const auto controller = Levye::Animation::CreateController();
    if (Levye::Animation::BindAnimation(controller, clips[0], 30.0f) &&
        Levye::Animation::PlayBound(controller)) {
        // Once per frame, after the host advances the controller clock:
        (void)Levye::Animation::ApplyBoundAnimation(controller, &model);
    }
    // For procedural edits, create a pose once, then each frame:
    const auto pose = Levye::Animation::CreatePose(controller, &model);
    if (Levye::Animation::SamplePose(pose)) {
        // Make edits here, then apply once.
        (void)Levye::Animation::ApplyPose(pose, &model);
    }
    Levye::Animation::DestroyPose(pose);
    Levye::Animation::DestroyController(controller);
    for (auto clip : clips) (void)Levye::Animation::UnloadAnimation(clip);
}
```

The example shows the call order; keep the controller and pose alive across frames in a real game. Call `ApplyBoundAnimation` for direct playback **or** `SamplePose`/`ApplyPose` for procedural playback in a frame.

## Positional and orientation IK

After `SamplePose`, use model-space targets and poles:

```cpp
const Vector3 target{0.8f, 1.0f, 0.2f};
const Vector3 pole{0.0f, 1.5f, 1.0f};
const bool solved = Levye::Animation::SolveTwoBoneIK(pose, root, middle, end, target, pole);
```

For two independent chains, fill two `AnimationIKChainRequest` values with unique IDs and validated root/middle/end indices, then call `Animation::SolveIKChains(pose, requests, 2, results)`. The solver orders chains by ID, rejects ownership conflicts, and reports each outcome in the corresponding result slot. A valid solve may report an unreachable target and clamp it; `Solved` and `reachable` answer different questions. Keep targets in model space, converting world contacts through the inverse caller-owned model transform.

```cpp
Levye::AnimationIKChainRequest chains[2]{};
chains[0].id = 1;
chains[0].root = leftRoot; chains[0].middle = leftMiddle; chains[0].end = leftEnd;
chains[0].target[0] = leftTarget.x; chains[0].target[1] = leftTarget.y;
chains[0].target[2] = leftTarget.z;
chains[1].id = 2;
chains[1].root = rightRoot; chains[1].middle = rightMiddle; chains[1].end = rightEnd;
chains[1].target[0] = rightTarget.x; chains[1].target[1] = rightTarget.y;
chains[1].target[2] = rightTarget.z;
Levye::AnimationIKChainResult outcomes[2]{};
(void)Levye::Animation::SolveIKChains(pose, chains, 2, outcomes);
```

For end-bone orientation after positional IK, set an `AnimationOrientationRequest` bone and an xyzw unit quaternion in `desired`, then call `Animation::SolveOrientations(pose, &request, 1, &result)`. A world-space request sets `worldSpace` and a valid `worldFromModel` quaternion. Orientation preserves the end bone origin, not an offset contact point. Apply the final pose once after both IK passes.

```cpp
Levye::AnimationOrientationRequest turn{};
turn.id = 3; turn.bone = end;
turn.desired[0] = desired.x; turn.desired[1] = desired.y;
turn.desired[2] = desired.z; turn.desired[3] = desired.w;
Levye::AnimationOrientationResult turnResult{};
(void)Levye::Animation::SolveOrientations(pose, &turn, 1, &turnResult);
```

## Contact, locomotion, and stabilization

`ContactController` in `<Levye/Modules/Animation/Contact/ContactTargets.hpp>` is caller-owned. Supply a world-space `ContactSurfaceHit` every update; after acquisition, `ContactState::Locked` holds a stable world anchor until release. For a moving rigid surface, enable `followSurface` and supply its matching `ContactSurfaceTransform`. Convert the contact to model-space IK with `ContactToIKRequest` or the inverse model transform before solving.

```cpp
Levye::ContactController contact;  // Keep this object across frames.
Levye::ContactConfig settings{};
Levye::ContactSurfaceHit hit{};     // Replace with a real world-space surface query.
hit.valid = true; hit.contactId = 1; hit.chainId = 1; hit.surfaceId = 1;
hit.position = worldFoot; hit.normal = {0, 1, 0};
if (contact.Update(animatedWorldFoot, hit, settings, deltaTime) &&
    contact.state == Levye::ContactState::Locked) {
    // contact.anchorWorld remains fixed until Release() or an accepted surface update.
}
```

`GaitScheduler`, `LocomotionTarget`, `PlanLanding`, and `EvaluateLimbReach` in `<Levye/Modules/Animation/Locomotion/Locomotion.hpp>` are generic caller-owned building blocks. Query terrain at the actual bounded footprint and keep a swing landing fixed from liftoff through touchdown. `BodyStabilizer` in `<Levye/Modules/Animation/Stabilization/BodyStabilization.hpp>` consumes world-space support contacts and reach limits; its result suggests a bounded offset and tilt. The caller owns movement intent, applies that adjustment to its root transform, and updates the model-space targets accordingly. It is kinematic, not a physics simulation.

```cpp
Levye::GaitScheduler gait;          // Configure once with validated GaitLimb entries.
Levye::LocomotionTarget foot;       // Keep one target per limb across frames.
(void)gait.Advance(deltaTime, cadence);
Levye::LandingPlan landing{};
(void)Levye::PlanLanding(intentRootWorld, limb.nominalLocal, rootVelocity,
                         stepDuration, limb.maxStrideReach, {0, 1, 0}, terrainHit, landing);
(void)foot.Update(limb, gait.States()[limbIndex], animatedWorldFoot, landing,
                  terrainHit, contactSettings, {0, 1, 0}, deltaTime);

Levye::BodyStabilizer stabilizer;   // Keep across frames; Reset() for a new gait.
Levye::StabilizationResult balance{};
(void)stabilizer.Update(deltaTime, intentBodyWorld, intentBodyWorld, headingWorld,
                        supports, supportCount, reaches, reachCount, balanceSettings, balance);
const Vector3 correctedRoot = Vector3Add(intentRootWorldPosition, balance.positionOffsetWorld);
// Apply correctedRoot and balance.orientationAdjustment to the caller-owned rig root.
```

The intended frame sequence is: **sample base pose → update contacts and locomotion → calculate stabilization → apply the caller-owned root adjustment → positional IK → orientation IK → apply final pose once**. Quaternions are xyzw; angles are radians. Keep inputs finite and normalized where required. See `AnimationAPI.hpp` for callback parameter ownership and [known limitations](Known-Issues-v0.4.5.md).
