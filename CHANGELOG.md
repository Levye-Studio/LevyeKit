# Changelog

All notable changes to LevyeKit are documented here.

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