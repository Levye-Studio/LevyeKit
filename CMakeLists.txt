cmake_minimum_required(VERSION 3.20)

option(
    LEVYE_BUILD_EXAMPLES
    "Build LevyeKit example projects"
    ON
)

option(
    LEVYE_BUILD_TOOLS
    "Build LevyeKit development tools"
    ON
)

option(LEVYE_BUILD_TESTS "Build input and available platform integration tests" OFF)

project(
    LevyeKit
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32 AND NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

include(FetchContent)

# Build raylib as a shared library.
#
# The Levye host and reloadable game module must use the same raylib
# instance. Linking independent static copies of raylib into both binaries
# could result in separate internal raylib state.
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)

set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)

FetchContent_MakeAvailable(raylib)

FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
)

FetchContent_MakeAvailable(tomlplusplus)

# ---------------------------------------------------------------------------
# LevyeKit
# ---------------------------------------------------------------------------

add_library(
    LevyeKit STATIC

    Levye/Core/Application.cpp

    Levye/HotReload/DynamicLibrary.cpp
    Levye/HotReload/GameModule.cpp

    Levye/Input/InputMap.cpp
    Levye/Screen/ScreenManager.cpp
    Levye/Assets/TextureManager.cpp
    Levye/Audio/AudioManager.cpp
    Levye/Graphics/ShaderManager.cpp
    Levye/Graphics/FontManager.cpp

    Levye/IO/FileWatcher.cpp
    Levye/Assets/AssetPath.cpp
    Levye/Platform/ExecutablePath.cpp
    Levye/Platform/SharedLibrary.cpp
    Levye/Time/TimeSystem.cpp
    Levye/Project/ProjectConfig.cpp
    Levye/Project/ProjectLoader.cpp

    Levye/Debug/Logger.cpp
)

target_include_directories(
    LevyeKit

    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    LevyeKit

    PUBLIC
        raylib
        tomlplusplus::tomlplusplus
        ${CMAKE_DL_LIBS}
)

# CMAKE_DL_LIBS contains the platform-specific library required for dynamic
# loading on platforms that need one.
target_link_libraries(
    LevyeKit

    PRIVATE
        ${CMAKE_DL_LIBS}
)

if(LEVYE_BUILD_EXAMPLES)

# ---------------------------------------------------------------------------
# Sandbox Game Module
# ---------------------------------------------------------------------------

add_library(
    Game SHARED

    Examples/Sandbox/Game/Game.cpp
)

target_include_directories(
    Game

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    Game

    PRIVATE
        raylib
)

set_target_properties(
    Game

    PROPERTIES
        PREFIX ""
        OUTPUT_NAME "Game"

        LIBRARY_OUTPUT_DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}/Targets/$<CONFIG>/lib"

        RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}/Targets/$<CONFIG>/lib"
)

# ---------------------------------------------------------------------------
# Sandbox Host
# ---------------------------------------------------------------------------

add_executable(
    Sandbox

    Examples/Sandbox/Main.cpp
)

target_link_libraries(
    Sandbox

    PRIVATE
        LevyeKit
)

set_target_properties(
    Sandbox

    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}/Targets/$<CONFIG>/bin"
)

# Copy shared raylib next to the Sandbox executable on Windows.
# Windows searches the executable directory for runtime DLL dependencies.
if(WIN32)
    add_custom_command(
        TARGET Sandbox
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:raylib>
            $<TARGET_FILE_DIR:Sandbox>
        COMMENT "Copying raylib DLL to Sandbox output directory"
        VERBATIM
    )
endif()

endif()

if(LEVYE_BUILD_TOOLS)
# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

add_executable(
    levye
    Tools/LevyeCLI/Main.cpp
    Tools/LevyeCLI/ProjectGenerator.cpp
    Tools/LevyeCLI/ProjectLocator.cpp
    Tools/LevyeCLI/BuildCommand.cpp
    Tools/LevyeCLI/RunCommand.cpp
    Tools/LevyeCLI/CleanCommand.cpp

    Levye/Project/ProjectConfig.cpp
    Levye/Project/ProjectLoader.cpp
    Levye/Platform/ExecutablePath.cpp
)

set_target_properties(
    levye
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_SOURCE_DIR}/Targets/$<CONFIG>/bin"
)

target_include_directories(
    levye
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    levye
    PRIVATE
        tomlplusplus::tomlplusplus
)

endif()