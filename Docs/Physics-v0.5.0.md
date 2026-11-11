# Physics module guide (API v7)

Enable the optional Jolt-backed Physics module with `-DLEVYE_WITH_PHYSICS=ON` for a direct framework build, or use `levye new MyGame --with physics` / `levye add physics` in a CLI-managed project. Include `<Levye/Modules/Physics/Physics.hpp>` in game code. The game module must have the appropriate host services bound; Physics is not available when the optional module is disabled or its callback API is incompatible.

For the application lifecycle, start with [`Templates/Default/Source/Main.cpp`](../Templates/Default/Source/Main.cpp) and [`Templates/Default/Source/Game.cpp`](../Templates/Default/Source/Game.cpp). The host owns simulation worlds, bodies, constraints, and virtual characters; game code retains generation-checked handles, not pointers to Jolt objects. Create the world and gameplay objects during initialization, retain handles in persistent state, and destroy them during normal shutdown. Compatible game-module hot reloads do not reconstruct the host-owned world.

## Setup and build

```sh
# New game project
levye new MyGame --with physics
cd MyGame
levye build
levye run
```

```sh
# Existing CLI-managed game project
levye add physics
levye build
```

For a direct CMake build from the framework repository:

```sh
cmake -S . -B Build/Physics -DLEVYE_WITH_PHYSICS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build Build/Physics -j 4
```

The generated `levye.project` module setting is `physics = true`. Reconfigure an existing CMake build directory after changing enabled modules.

## Worlds and fixed stepping

`PhysicsWorldDesc` configures gravity and Jolt capacity limits for bodies, body pairs, and contacts. The host `TimeSystem` determines the fixed update rate shared by Physics worlds. Create worlds during normal initialization and destroy them on shutdown; avoid creating a world every frame.

A world can be paused with `Physics::SetWorldPaused()` and scheduled to advance once on the next host fixed tick with `Physics::StepWorldOnce()`. Physics operations, stepping, and event polling must run on the host update thread and not concurrently with a simulation step. Do not independently step Jolt from game-module callbacks.

`Physics::GetWorldDiagnostics()` reports active bodies, characters, contact pairs, queued events, rejected creations, completed steps, and last-step wall-clock duration. The timing is diagnostic rather than a deterministic simulation metric.

## Rigid bodies and collision shapes

`PhysicsBodyDesc` specifies a static, dynamic, or kinematic body and its mass, friction, restitution, collision layer/mask, and sensor status. Supported primitive shapes are boxes, spheres, and capsules. Additional shapes include:

| Shape | Supported usage | Restrictions |
|---|---|---|
| Convex hull | Bodies and supported spatial queries | 4–256 finite vertices |
| Triangle mesh | Static-body collision and raycast hits | Not valid for dynamic/kinematic bodies or mesh overlap input |
| Compound | Bodies and overlap queries | Up to 32 transformed children; no compound shape-cast input |

Dynamic and kinematic compounds require convex children; static compounds may contain meshes. Nested compounds deeper than four levels are rejected. Body and child transforms require unit scale. Unsupported combinations, invalid mesh indices, and nonfinite vertices are rejected rather than silently accepted.

Shape source arrays and compound child descriptors are copied into host-owned Jolt shapes during the creation call. The host does not retain pointers into the reloadable game module.

## Collision filtering, contacts, and spatial queries

Layers and masks determine which bodies can interact; sensors report interaction without ordinary physical collision response. Contact events are queued for the game to inspect. Keep event polling on the host update thread, and do not treat an event buffer as persistent storage across reloads.

Spatial queries include raycasts, shape casts, and overlap tests:

- `Physics::ShapeCast()` sweeps a supported sphere, box, capsule, or convex hull from a unit-scale transform along a normalized direction for a maximum distance. The closest accepted body is returned. The hit fraction is traveled distance divided by the requested maximum distance; an initial penetration reports zero.
- `Physics::OverlapShape()` reports unique bodies in ascending public-handle order. On success, the returned count is the **total** number of hits, even if the caller's output capacity is smaller. Only the first `min(count, capacity)` handles are written. A null output pointer is valid with zero capacity.
- `PhysicsQueryFilter` selects body layers, can exclude sensors, and can ignore one live body from the same world.

Queries do not advance the world or modify contact events. Triangle meshes and compounds cannot be used as shape-cast inputs, and triangle meshes cannot be used as overlap inputs.

## Constraints

`Physics::CreateConstraint()` joins two live bodies in the same world. The module supports fixed, point-to-point, hinge, and slider constraints. Fixed constraints preserve the bodies' current relative transform. Other constraints use a world-space anchor; hinge and slider use a world-space axis, and hinge also uses a perpendicular normal to establish zero angle.

Hinge limits are in **radians**; slider limits are in **metres**. Supported hinge and slider motors use velocity or position targets and a force/torque limit; `Physics::SetConstraintMotor()` updates a motor at runtime. Connected bodies do not collide by default; enable `collideConnected` when those contacts are desired.

Constraints are host-owned and survive compatible game-module reload. Destroying a connected body or world removes its constraints; stale constraint handles are not reusable.

## Virtual character controller

`Physics::CreateCharacter()` creates a Y-up capsule. Its `height` includes both hemispheres, and `position` is the capsule center. `Physics::SetCharacterVelocity()` receives the player's horizontal world-space intent, so its Y component must be zero. The fixed Physics step resolves gravity and collisions. `Physics::GetCharacterVelocity()` returns measured movement, including vertical velocity.

`Physics::JumpCharacter()` starts a jump from walkable ground. `Physics::SetCharacterPosition()` is intended for explicit teleports or resets. Ground states are `Grounded`, `Sliding` on slopes beyond the configured limit, and `Airborne`. The controller supports stair traversal, ground snapping, dynamic-body interaction, and kinematic moving platforms.

Kinematic platforms advance before the virtual character's fixed step. While grounded, a character inherits its support body's current velocity, including platform direction changes; jumping preserves horizontal platform momentum until landing. Continue to pass player intent separately through `SetCharacterVelocity()`.

**Current limitations:** virtual characters do not collide with one another; they ignore sensors; and only the Y component of world gravity accelerates characters. A character's movement is evaluated at the fixed timestep, so changing platform velocity between ticks can affect the result.

## Sandbox verification

Build with Physics enabled, then launch the Sandbox executable produced by your build. For the existing Physics build used during v0.5.0 validation:

```sh
./Targets/build-physics/bin/Sandbox
```

In the Physics scene:

| Key | Action |
|---|---|
| `G` | Sphere cast |
| `O` | Overlap query |
| `F` | Toggle ball sensor interaction |
| `Tab` | Switch to character course |
| `WASD` | Move character |
| `Shift` | Sprint |
| `Space` | Jump |
| `R` | Reset character |
| `P` | Pause/resume Physics world |
| `.` | Advance one fixed step while paused |
| `B` | Toggle contact/support debug markers |

The character course includes a wall, low step, steep slope, and raised moving platform. To inspect platform stability, switch to character mode, reset, release movement keys, and wait until grounded on the platform. Observe at least ten platform reversals; the idle-drift diagnostic resets after movement, loss of support, or character reset.

The HUD displays world counters and step timing. Debug drawing is performed by Sandbox, not from inside Jolt simulation callbacks.

## Hot reload, ABI, and cleanup

The Physics callback ABI is `PHYSICS_API_VERSION` **7**, separate from the LevyeKit release version and `GAME_API_VERSION`. Physics API v7 changes descriptor layouts: **rebuild game modules compiled against Physics API v6**. The host rejects incompatible Physics callback versions. Game API and HostServices interfaces were not changed by the v0.5.0 Physics work covered here.

Retain only valid Physics handles in persistent game state. The host owns the underlying objects and retains them across compatible reloads. Destroy characters, constraints, bodies, and worlds as appropriate during final shutdown. If the layout of the game's own persistent `GameState` changes, restart the application instead of relying on hot reload.

## Validation and known limitations

The LevyeKit library and Physics Sandbox built successfully, and the graphical Sandbox was manually checked. Linux x86_64 and Windows have **not** been validated for this release.

Additional restrictions to account for when designing gameplay:

- Triangle mesh collision bodies must be static.
- Virtual characters do not collide with other virtual characters.
- Character gravity uses Y-up acceleration.
- Physics operations must not race a running simulation step.
- Invalid shape and descriptor inputs are rejected.

For the complete descriptor fields and exact function signatures, consult the checked-out public headers under `Levye/Modules/Physics/`; this guide intentionally describes call order and contracts rather than guessing initializer layouts.
