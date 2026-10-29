# Changelog

All notable changes to LevyeKit are documented here.

## [0.4.0] - 2026-10-29

### Added
- Optional host-owned skeletal Animation module (`ANIMATION_API_VERSION = 9`) with generation-safe handles for clips and controllers.
- Animation playback controls for looping, pause/resume, playback speed, and restart.
- Interruptible crossfades with cached-pose transitions to reduce visible discontinuities.
- Named animation states with priorities, one-shot playback, automatic return states, and state-machine controls.
- Conditional state transitions using Boolean, float, integer, and trigger parameters.
- Seven Sandbox animation verification modes covering playback, crossfades, states, conditions, one-shots, gameplay, and resource lifecycle/reload.
- Platformer3D integration demo in LevyeKit-Examples, exercising Animation, ImGui, YAML serialization, and hot reload together.

### Improved
- Animation controllers, clips, playback state, and transition data remain host-owned across compatible game-module hot reloads.
- Animation regression coverage and public API documentation.
- Cross-platform validation of framework builds, game execution, and hot reload on macOS, Arch Linux, and Windows (MSVC).
- Portable LevyeKit checkout resolution for the Platformer3D example instead of a machine-specific absolute path.

### Fixed
- yaml-cpp 0.8.0 compilation with newer GCC versions by ensuring `<cstdint>` is included during dependency compilation; avoids the `uint16_t` build error encountered on Arch Linux.
- CMake dependency integration preserves the `yaml-cpp::yaml-cpp` target when applying the compiler compatibility workaround.

### Validation
- Linux: LevyeKit build, CLI, framework tests, generated-project workflow, runtime, and hot reload passed; Platformer3D gameplay, ImGui, Animation, and manual/archive serialization were exercised successfully.
- Windows: LevyeKit and Platformer3D builds, launches, and hot reload passed with MSVC.
- macOS: LevyeKit and Platformer3D builds, launches, and hot reload passed.

### Compatibility
- Game API version: `15`.
- Serialization API version: `6`.
- ImGui API version: `1`.
- Animation API version: `9`.
- Animation, Serialization, and ImGui remain optional modules.

## [0.3.0] - 2026-10-09

### Added
- Optional Dear ImGui integration using rlImGui.
- Host-owned ImGui context and frame lifecycle.
- Dear ImGui support across game-module hot reloads.
- ImGui module API (`IMGUI_API_VERSION = 1`).
- `levye new --with imgui` and `levye add imgui` support.
- ImGui configuration in generated project templates.
- Keyboard and mouse input-capture integration.
- Hardware-independent input-capture tests.

### Improved
- Input handling when Dear ImGui captures keyboard or mouse input.
- Input transition handling to prevent artificial press and release events during capture changes.
- Gamepad input remains available while ImGui captures keyboard or mouse input.
- Documentation for optional modules, input capture, and ImGui integration.

### Fixed
- Windows MSVC DLL linking for Dear ImGui by exporting symbols from `LevyeDearImGui`.

### Compatibility
- Game API version remains `15`.
- Serialization API version remains `6`.
- ImGui API version is `1`.
- Dear ImGui remains optional and disabled by default.