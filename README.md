<a id="readme-top"></a>

<!-- PROJECT SHIELDS -->

[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]

<!-- PROJECT LOGO -->

<br />

<div align="center">
  <a href="https://github.com/Levye-Studio/LevyeKit">
    <img src="images/logo.png" alt="LevyeKit logo" width="80" height="80">
  </a>

  <h3 align="center">LevyeKit</h3>

  <p align="center">
    A lightweight C++ game framework built on raylib for Levye Studio games.
    <br />
    <a href="#usage-api"><strong>Explore the docs »</strong></a>
    <br />
    <br />
    <a href="#getting-started">Get Started</a>
    &middot;
    <a href="https://github.com/Levye-Studio/LevyeKit/issues/new?labels=bug">Report Bug</a>
    &middot;
    <a href="https://github.com/Levye-Studio/LevyeKit/issues/new?labels=enhancement">Request Feature</a>
  </p>
</div>

<!-- TABLE OF CONTENTS -->

<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
        <li><a href="#design-philosophy">Design Philosophy</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
        <li><a href="#creating-a-game">Creating a Game</a></li>
      </ul>
    </li>
    <li>
      <a href="#levye-cli">Levye CLI</a>
    </li>
    <li>
      <a href="#usage-api">Usage API</a>
      <ul>
        <li><a href="#public-api">Public API</a></li>
        <li><a href="#architecture">Architecture</a></li>
        <li><a href="#game-api">Game API</a></li>
        <li><a href="#hot-reloading">Hot Reloading</a></li>
        <li><a href="#state-preservation">State Preservation</a></li>
        <li><a href="#raylib-access">raylib Access</a></li>
        <li><a href="#repository-layout">Repository Layout</a></li>
      </ul>
    </li>
    <li><a href="#platforms">Platforms</a></li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
  </ol>
</details>

<!-- ABOUT THE PROJECT -->

## About The Project

LevyeKit is a lightweight C++ game development framework built on top of [raylib](https://www.raylib.com/).

It is being developed as the reusable foundation for games made by **Levye Studio**.

The goal is not to replace raylib or become another large game engine. LevyeKit handles the common project infrastructure that would otherwise need to be recreated for every game while keeping raylib directly accessible to game code.

LevyeKit provides reusable systems for:

* Application lifecycle
* Native C++ game modules
* Code hot reloading
* Persistent game state
* Input management
* Texture, shader, and font management
* Asset hot reloading
* Audio and music management
* Screen management
* Time and fixed-step simulation
* Project configuration
* Debug and Release configurations
* Project generation
* Command-line development workflow

The framework stays intentionally small so games can use raylib directly whenever it already provides the required functionality.

> **Status:** LevyeKit `v0.1` is the first game-ready development milestone. The framework is still evolving and APIs may change in future releases.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

### Built With

* [![C++][C++-shield]][C++-url]
* [raylib](https://www.raylib.com/)
* [CMake](https://cmake.org/)

<p align="right">(<a href="#readme-top">back to top</a>)</p>

### Design Philosophy

LevyeKit should stay small.

If raylib already provides something cleanly, LevyeKit should generally use it rather than wrapping it unnecessarily.

The framework exists to solve repeated infrastructure problems across Levye Studio games, not to hide the underlying technology.

Its main goals are:

* Keep raylib directly accessible
* Avoid repeating project setup for every game
* Provide a reusable application lifecycle
* Support native C++ hot reloading during development
* Preserve game state across compatible code reloads
* Keep resources owned by the stable application host
* Provide reusable input, audio, asset, shader, font, screen, and time systems
* Make creating and running a new game fast
* Keep the framework understandable
* Add systems only when games actually need them

LevyeKit deliberately avoids trying to become a full editor-driven engine.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- GETTING STARTED -->

## Getting Started

### Prerequisites

LevyeKit currently requires:

* Git
* CMake
* A C++20 compatible compiler

Platform-specific development tools may also be required depending on the target platform.

### Installation

Clone the repository:

```sh
git clone https://github.com/Levye-Studio/LevyeKit.git
cd LevyeKit
```

Configure a Debug build:

```sh
cmake -S . -B Build/Debug -DCMAKE_BUILD_TYPE=Debug
```

Build LevyeKit and its development tools:

```sh
cmake --build Build/Debug
```

The development CLI is built as:

```text
levye
```

Check the installed version:

```sh
levye --version
```

Or view the available commands:

```sh
levye --help
```

The included Sandbox can also be used to test framework systems and hot reloading.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

### Creating a Game

Create a new LevyeKit game:

```sh
levye new MyGame
```

Create one and initialize a Git repository:

```sh
levye new MyGame --git
```

Or initialize Git and create the first commit:

```sh
levye new MyGame --commit
```

Then enter the generated project:

```sh
cd MyGame
```

Build it:

```sh
levye build
```

Run it:

```sh
levye run
```

A generated project contains the basic structure required to immediately start writing game code:

```text
MyGame/
├── Assets/
├── Source/
│   ├── Game.cpp
│   └── Main.cpp
├── .gitignore
├── CMakeLists.txt
└── levye.project
```

Game-specific settings such as the project name, target, window configuration, and asset directory are stored in `levye.project`.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- LEVYE CLI -->

## Levye CLI

LevyeKit includes a small command-line tool for the normal game-development workflow.

```text
levye new <name> [options]
levye build [options]
levye run [options]
levye clean [options]
levye project

levye --help
levye --version
```

### Create

```sh
levye new MyGame
```

Optional Git integration:

```sh
levye new MyGame --git
levye new MyGame --commit
```

### Build

Debug is the default configuration:

```sh
levye build
```

For Release:

```sh
levye build --release
```

### Run

Run an existing Debug build:

```sh
levye run
```

Or Release:

```sh
levye run --release
```

`levye run` does not implicitly rebuild the game. Build and run remain separate operations.

### Clean

Clean Debug output:

```sh
levye clean
```

Clean Release:

```sh
levye clean --release
```

Clean all generated build configurations:

```sh
levye clean --all
```

### Project

Display information about the current project:

```sh
levye project
```

The CLI searches parent directories for `levye.project`, allowing commands to be used from directories inside a game project.

Command-specific help is also available:

```sh
levye new --help
levye build --help
levye run --help
levye clean --help
levye project --help
```

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- USAGE API -->

## Usage API

### Public API

Game code can include the main LevyeKit header:

```cpp
#include <Levye/LevyeKit.hpp>
```

The primary game-facing APIs are:

```cpp
Levye::Log
Levye::Input
Levye::Screen
Levye::Time
Levye::Assets
Levye::Audio
```

For example:

```cpp
Levye::Log::Info("Game started.");

if (Levye::Input::IsPressed("Action"))
{
    // Handle the action.
}

Levye::Screen::Set("Gameplay");

Levye::Time::SetPaused(false);
```

Assets are represented by lightweight handles while the actual resources remain owned by the host:

```cpp
Levye::AssetHandle texture =
    Levye::Assets::LoadTexture("player.png");

const Texture2D* player =
    Levye::Assets::GetTexture(texture);
```

Audio follows the same model:

```cpp
Levye::AssetHandle sound =
    Levye::Audio::LoadSound("click.wav");

Levye::Audio::PlaySound(sound);
```

Individual LevyeKit headers remain available when a game prefers narrower includes.

### Input

LevyeKit uses an **action-based input system**.

Before an action can be queried from game code, it must first be bound to one or more inputs.

For example, an action named `Jump` can be bound to the Space key:

```cpp
input.BindKey("Jump", KEY_SPACE);
```

Multiple inputs can be bound to the same action:

```cpp
input.BindKey("Jump", KEY_SPACE);
input.BindGamepadButton(
    "Jump",
    GAMEPAD_BUTTON_RIGHT_FACE_DOWN
);
```

Game code can then query the action through the public `Levye::Input` API:

```cpp
if (Levye::Input::IsPressed("Jump"))
{
    // Jump.
}

if (Levye::Input::IsDown("Jump"))
{
    // The action is currently held.
}

if (Levye::Input::IsReleased("Jump"))
{
    // The action was released this frame.
}
```

Analog or directional actions can also be queried as axes:

```cpp
const float horizontal =
    Levye::Input::GetAxis("MoveHorizontal");
```

The binding itself is owned by the host-side input system, while gameplay code queries actions through `Levye::Input`.

This keeps gameplay code independent from specific keys or controller buttons:

```text
KEY_SPACE
    |
    v
  "Jump"
    |
    v
Levye::Input::IsPressed("Jump")
    |
    v
Gameplay
```

This also allows several physical inputs to represent the same gameplay action without changing the gameplay code.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

### Architecture

LevyeKit separates the stable application host from reloadable game-specific code.

```text
                    Levye Host
                        |
        +---------------+---------------+
        |                               |
        v                               v
+------------------+             +-------------+
| Host Systems     |             | Game State  |
|                  |             |             |
| Window / raylib  |             | Persistent  |
| Input            |             | game data   |
| Assets           |             +-------------+
| Audio            |
| Shaders          |
| Fonts            |
| Screens          |
| Time             |
+--------+---------+
         |
         | HostServices
         v
+------------------+
| Game Module      |
|                  |
| Gameplay         |
| Game Logic       |
| Rendering        |
+--------+---------+
         |
         v
    Game.dylib
    Game.so
    Game.dll
```

The host owns long-lived systems and remains running while game-specific code can be rebuilt and replaced.

Game code accesses host-owned systems through LevyeKit's public facades:

```text
Game Code
    |
    v
Levye::Input / Assets / Audio / Time / Screen / Log
    |
    v
Host Services
    |
    v
Stable Host Systems
```

This keeps reloadable game code separated from the implementation of long-lived framework systems.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

### Game API

The host communicates with the reloadable game module through a small callback table.

A game module provides lifecycle callbacks such as:

```cpp
void OnLoad(void* state);
void OnBeforeReload(void* state);
void OnAfterReload(void* state);

void OnUpdate(void* state, float deltaTime);
void OnFixedUpdate(void* state, float fixedDeltaTime);
void OnDraw(void* state);

void OnShutdown(void* state);
```

Persistent state is explicitly constructed and destroyed:

```cpp
void InitializeState(void* state);
void DestroyState(void* state);
```

Host services are bound separately from gameplay callbacks:

```cpp
void BindServices(
    const Levye::HostServices* services
);
```

The game module exports a C-compatible entry point:

```cpp
extern "C"
const Levye::GameAPI* GetGameAPI();
```

A typical export looks like:

```cpp
extern "C"
const Levye::GameAPI* GetGameAPI()
{
    static const Levye::GameAPI api = {
        .version = Levye::GAME_API_VERSION,

        .BindServices = BindServices,

        .OnLoad = OnLoad,

        .OnBeforeReload = OnBeforeReload,
        .OnAfterReload = OnAfterReload,

        .OnUpdate = OnUpdate,
        .OnFixedUpdate = OnFixedUpdate,
        .OnDraw = OnDraw,
        .OnShutdown = OnShutdown,

        .InitializeState = InitializeState,
        .DestroyState = DestroyState,

        .stateSize = sizeof(GameState)
    };

    return &api;
}
```

`extern "C"` gives the exported function a predictable symbol name that can be located by the dynamic-library loader.

`GAME_API_VERSION` is independent from the LevyeKit release version. It represents compatibility of the hot-reload API boundary.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

### Hot Reloading

Native C++ hot reloading is a core part of LevyeKit's desktop development workflow.

Game code is compiled into a shared library:

| Platform | Game Module |
| -------- | ----------- |
| macOS | `Game.dylib` |
| Linux | `Game.so` |
| Windows | `Game.dll` |

The development loop is:

```text
Start game
    |
    v
Host opens the raylib window
    |
    v
Load Game module
    |
    v
Game running
    |
    +---- Edit game code
    |
    +---- levye build
    |
    v
Detect new Game module
    |
    v
Validate candidate
    |
    v
OnBeforeReload
    |
    v
Unload old game code
    |
    v
Load new game code
    |
    v
Bind host services
    |
    v
OnAfterReload
    |
    v
Continue running
```

The application, raylib window, host systems, and compatible persistent game state remain alive while game code is replaced.

The build output itself is not loaded directly. LevyeKit loads a runtime copy of the shared library so the build system remains free to replace the normal output during development.

If a new module is invalid, LevyeKit rejects it instead of replacing the currently running game module.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

### State Preservation

Game code and game state are deliberately separated.

```text
Levye Host
    |
    +---- GameState      <- host-owned memory
    |        |
    |        +---- survives compatible reloads
    |
    +---- Game module    <- replaceable code
```

The game defines its own concrete state:

```cpp
struct GameState
{
    Vector2 playerPosition{};
    float playerSpeed = 200.0f;

    Levye::AssetHandle playerTexture{};
};
```

The host only needs to know the amount of memory required:

```cpp
.stateSize = sizeof(GameState)
```

State is constructed once:

```cpp
void InitializeState(void* state)
{
    new (state) GameState{};
}
```

and destroyed during final shutdown:

```cpp
void DestroyState(void* state)
{
    static_cast<GameState*>(state)->~GameState();
}
```

Normal hot reloads do not reconstruct the state.

If `sizeof(GameState)` changes while the application is running, LevyeKit rejects the reload and requires the game to restart.

For `v0.1`, changing the layout of `GameState` should therefore be treated as a restart-required change.

Game state should store lightweight LevyeKit handles rather than copies or pointers to host-owned resources.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

### raylib Access

LevyeKit does not hide raylib behind another rendering API.

Game modules can use normal raylib functionality directly:

```cpp
void OnDraw(void* state)
{
    (void)state;

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

LevyeKit manages the outer application lifecycle while the game remains in control of its rendering and behavior.

The host owns the raylib window and frame lifecycle:

```text
BeginDrawing
    |
    v
Game OnDraw
    |
    v
EndDrawing
```

This keeps LevyeKit lightweight while preserving the simplicity of raylib.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

### Repository Layout

The repository is organized around the framework, tools, templates, examples, and platform support.

```text
LevyeKit/
├── Levye/
│   ├── Assets/
│   ├── Audio/
│   ├── Core/
│   ├── Debug/
│   ├── Graphics/
│   ├── HotReload/
│   ├── IO/
│   ├── Input/
│   ├── Platform/
│   ├── Project/
│   ├── Screen/
│   └── Time/
│
├── Examples/
│   └── Sandbox/
│
├── Templates/
│   └── Default/
│
├── Tools/
│   └── LevyeCLI/
│
├── Platform/
├── .vscode/
├── CMakeLists.txt
└── README.md
```

`Levye/` contains the reusable framework.

`Examples/Sandbox/` is used to exercise framework systems and the game-module architecture.

`Templates/Default/` contains the starter project copied by `levye new`.

`Tools/LevyeCLI/` contains the Levye command-line development tool.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- PLATFORMS -->

## Platforms

LevyeKit is intended to provide a common foundation across desktop and mobile targets.

| Platform | Target | Status |
| -------- | ------ | ------ |
| macOS | Desktop | Development platform |
| Linux | Desktop | Planned / evolving |
| Windows | Desktop | Planned / evolving |
| iOS | Mobile | Planned |
| Android | Mobile | Planned |

Native code hot reloading is primarily intended for desktop development.

Mobile platforms may use a different development workflow because of platform restrictions around dynamically replacing executable code.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- ROADMAP -->

## Roadmap

### v0.1 — Game Ready

* [x] CMake project
* [x] raylib integration
* [x] Application lifecycle
* [x] Game API boundary
* [x] Dynamic library loading
* [x] Native game-module hot reload
* [x] Persistent host-owned game state
* [x] Hot-reload compatibility checks
* [x] File watching
* [x] Logging
* [x] Input system
* [x] Screen management
* [x] Time and fixed-step simulation
* [x] Texture management
* [x] Shader management
* [x] Font management
* [x] Audio and music management
* [x] Asset hot reloading
* [x] Public game-facing API
* [x] `LevyeKit.hpp` umbrella header
* [x] Project configuration
* [x] Debug and Release configurations
* [x] Game project template
* [x] `levye new`
* [x] `levye build`
* [x] `levye run`
* [x] `levye clean`
* [x] `levye project`
* [x] CLI help and version information
* [x] Optional Git initialization for generated projects

### Future

* [ ] Continue Linux support
* [ ] Continue Windows support
* [ ] iOS support
* [ ] Android support
* [ ] Improve failed-reload diagnostics
* [ ] Expand the framework only as real games require new reusable systems

LevyeKit intentionally does not currently plan to become a full editor-driven engine.

See the [open issues][issues-url] for proposed features and reported bugs.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- CONTRIBUTING -->

## Contributing

LevyeKit is still evolving, so focused contributions and bug reports are welcome.

1. Fork the project.
2. Create a branch for your change.
3. Build and test the Sandbox.
4. Keep changes focused on LevyeKit's lightweight design.
5. Avoid unnecessary abstractions when raylib already provides a suitable solution.
6. Open a pull request against `main`.

For bugs, include steps to reproduce the issue and describe the platform and build configuration used.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- LICENSE -->

## License

License to be determined.

Third-party libraries used by LevyeKit retain their respective licenses.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- CONTACT -->

## Contact

[Levye Studio](https://github.com/Levye-Studio)

Project: [LevyeKit](https://github.com/Levye-Studio/LevyeKit)

For bugs and feature requests, use the [issue tracker][issues-url].

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- ACKNOWLEDGMENTS -->

## Acknowledgments

* [raylib](https://www.raylib.com/) and its contributors
* [CMake](https://cmake.org/)
* The maintainers of LevyeKit's third-party dependencies
* [Best-README-Template](https://github.com/othneildrew/Best-README-Template) for the README layout
* [Shields.io](https://shields.io/) for the badges

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- MARKDOWN LINKS & IMAGES -->

[contributors-shield]: https://img.shields.io/github/contributors/Levye-Studio/LevyeKit.svg?style=for-the-badge
[contributors-url]: https://github.com/Levye-Studio/LevyeKit/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/Levye-Studio/LevyeKit.svg?style=for-the-badge
[forks-url]: https://github.com/Levye-Studio/LevyeKit/forks
[stars-shield]: https://img.shields.io/github/stars/Levye-Studio/LevyeKit.svg?style=for-the-badge
[stars-url]: https://github.com/Levye-Studio/LevyeKit/stargazers
[issues-shield]: https://img.shields.io/github/issues/Levye-Studio/LevyeKit.svg?style=for-the-badge
[issues-url]: https://github.com/Levye-Studio/LevyeKit/issues
[C++-shield]: https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=cplusplus&logoColor=white
[C++-url]: https://isocpp.org/