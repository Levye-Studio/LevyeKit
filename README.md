# LevyeKit

LevyeKit is a lightweight C++ game development framework built on top of [raylib](https://www.raylib.com/).

It is being developed as the reusable foundation for games made by **Levye Studio**.

The goal is not to replace raylib or build another large game engine. LevyeKit handles the common project infrastructure that would otherwise need to be recreated for every game while keeping raylib directly accessible to game code.

## Status

**Early development — v0.1**

LevyeKit is currently establishing its core application architecture and dynamic game-module system.

The API is expected to change while the framework is being developed.

## Goals

LevyeKit is designed around a few main goals:

* Keep raylib directly accessible
* Avoid repeating project setup for every game
* Provide a reusable application lifecycle
* Support native C++ hot reloading during development
* Preserve game state across code reloads where possible
* Provide reusable input, audio and asset systems
* Support desktop and eventually mobile platforms
* Make creating a new Levye game fast
* Keep the framework small and understandable

## Architecture

LevyeKit separates the application host from game-specific code.

```text
                 LevyeHost
                     |
                     v
              +-------------+
              |  LevyeKit   |
              |             |
              | Window      |
              | Main Loop   |
              | Input       |
              | Assets      |
              | Audio       |
              | Hot Reload  |
              +------+------+
                     |
                  GameAPI
                     |
                     v
              +-------------+
              | Game Module |
              |             |
              | Gameplay    |
              | Screens     |
              | Entities    |
              | Game Logic  |
              +-------------+
                     |
                     v
              Game.dylib
              Game.so
              Game.dll
```

The host application stays running while game-specific code can eventually be rebuilt and reloaded.

## Game API

LevyeKit communicates with game code through a small function table.

A game module implements callbacks such as:

```cpp
void OnLoad(Levye::GameState* state);
void OnUpdate(Levye::GameState* state, float deltaTime);
void OnDraw(Levye::GameState* state);
void OnUnload(Levye::GameState* state);
```

The module exposes a single C-compatible entry point:

```cpp
extern "C" Levye::GameAPI GetGameAPI();
```

Using `extern "C"` gives the exported function a predictable symbol name that can be located by the host's dynamic-library loader.

## Hot Reloading

Hot reloading is a core design goal rather than an optional feature added later.

During development, game-specific code is compiled into a shared library:

```text
macOS      Game.dylib
Linux      Game.so
Windows    Game.dll
```

The Levye host will eventually watch this file for changes.

The intended workflow is:

```text
Start game
    |
    v
LevyeHost opens the raylib window
    |
    v
Load Game module
    |
    v
Game running
    |
    +---- Edit game code
    |
    +---- Rebuild Game module
    |
    v
LevyeKit detects the new module
    |
    v
Unload old game code
    |
    v
Load new game code
    |
    v
Continue running
```

The raylib window should remain open during this process.

## State Preservation

Game code and game state are deliberately treated separately.

```text
LevyeHost
    |
    +---- GameState      <- remains alive
    |
    +---- Game.dylib     <- replaceable
```

This makes it possible for future hot reloads to replace functions without necessarily restarting the entire game.

The current `GameState` is only a placeholder while this system is developed.

## raylib

LevyeKit does not attempt to hide raylib.

Game modules can still use the normal raylib API:

```cpp
void OnDraw(Levye::GameState* state)
{
    ClearBackground(BLACK);

    DrawText(
        "Hello from LevyeKit!",
        40,
        40,
        32,
        RAYWHITE
    );
}
```

LevyeKit manages the outer application lifecycle while the game controls its actual rendering and behavior.

The host owns the frame boundaries:

```cpp
BeginDrawing();

game.OnDraw();

EndDrawing();
```

## Project Structure

Current structure:

```text
LevyeKit/
├── Levye/
│   ├── Core/
│   │   ├── Application.hpp
│   │   ├── Application.cpp
│   │   └── GameAPI.hpp
│   │
│   └── HotReload/
│       ├── DynamicLibrary.hpp
│       └── DynamicLibrary.cpp
│
├── Platform/
│   ├── Linux/
│   ├── macOS/
│   └── Windows/
│
├── Templates/
│   └── Default/
│
├── Examples/
│   └── Sandbox/
│       ├── Main.cpp
│       └── Game/
│           └── Game.cpp
│
├── Scripts/
├── Targets/
├── .vscode/
├── CMakeLists.txt
└── README.md
```

This structure will evolve as the framework grows.

## Building

LevyeKit currently uses CMake and requires a C++20 compiler.

Configure a debug build:

```bash
cmake -S . -B Build -DCMAKE_BUILD_TYPE=Debug
```

Build:

```bash
cmake --build Build -j
```

Run the Sandbox:

```bash
./Targets/Debug/bin/Sandbox
```

## Planned Systems

The current development roadmap includes:

* [x] CMake project
* [x] raylib integration
* [x] Application lifecycle
* [x] Game API boundary
* [x] Dynamic library loading
* [x] macOS dynamic game module
* [ ] GameModule abstraction
* [ ] Automatic code hot reload
* [ ] State preservation across reloads
* [ ] Failed-reload recovery
* [ ] Asset hot reload
* [ ] Shader hot reload
* [ ] Input system
* [ ] Audio system
* [ ] Asset manager
* [ ] Screen management
* [ ] Windows support
* [ ] Linux support
* [ ] iOS support
* [ ] Android support
* [ ] Game project templates
* [ ] `levye new <GameName>` project generator

## Platforms

Planned platform support:

| Platform | Status         |
| -------- | -------------- |
| macOS    | In development |
| Linux    | Planned        |
| Windows  | Planned        |
| iOS      | Planned        |
| Android  | Planned        |

Native code hot reloading is primarily intended for desktop development. Mobile platforms may use a different development workflow because of platform restrictions around dynamically replacing executable code.

## Philosophy

LevyeKit should stay small.

If raylib already provides something cleanly, LevyeKit should generally use it rather than wrap it unnecessarily.

The framework exists to solve repeated infrastructure problems across Levye Studio games, not to hide the underlying technology.

## License

License to be determined.
