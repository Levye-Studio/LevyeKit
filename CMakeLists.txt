cmake_minimum_required(VERSION 3.20)

project(
    LevyeKit
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

# ---------------------------------------------------------------------------
# LevyeKit
# ---------------------------------------------------------------------------

add_library(
    LevyeKit STATIC

    Levye/Core/Application.cpp

    Levye/HotReload/DynamicLibrary.cpp
    Levye/HotReload/GameModule.cpp
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
)

# CMAKE_DL_LIBS contains the platform-specific library required for dynamic
# loading on platforms that need one.
target_link_libraries(
    LevyeKit

    PRIVATE
        ${CMAKE_DL_LIBS}
)

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