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

option(
    LEVYE_WITH_SERIALIZATION
    "Build the optional LevyeKit serialization module"
    OFF
)

option(
    LEVYE_WITH_IMGUI
    "Enable the optional Dear ImGui development tools module"
    OFF
)

project(
    LevyeKit
    VERSION 0.2.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Default build configuration
# ---------------------------------------------------------------------------

# Single-configuration generators (Ninja and Unix Makefiles) require
# CMAKE_BUILD_TYPE to be set explicitly.
#
# Multi-configuration generators (Xcode, Visual Studio and Ninja
# Multi-Config) select the configuration at build time instead.
#
# Default to Debug on every platform when no configuration was
# explicitly selected.

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(
        CMAKE_BUILD_TYPE
        Debug
        CACHE STRING "Build type"
        FORCE
    )
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

# ---------------------------------------------------------
# Optional modules
# ---------------------------------------------------------

if(LEVYE_WITH_SERIALIZATION)
    add_subdirectory(
        Levye/Modules/Serialization
    )

    # yaml-cpp may be shared; Windows does not use build-tree rpaths.
    function(levye_copy_serialization_runtime target)
        if(WIN32)
            get_target_property(yaml_type yaml-cpp TYPE)
            if(yaml_type STREQUAL "SHARED_LIBRARY")
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        $<TARGET_FILE:yaml-cpp> $<TARGET_FILE_DIR:${target}>
                    VERBATIM)
            endif()
        endif()
    endfunction()

    target_link_libraries(
        LevyeKit

        PRIVATE
            LevyeSerialization
    )

    target_compile_definitions(
        LevyeKit

        PUBLIC
            LEVYE_WITH_SERIALIZATION=1
    )
endif()

if(LEVYE_WITH_IMGUI)
    include(FetchContent)

    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG master
    )

    FetchContent_Declare(
        rlimgui
        GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
        GIT_TAG main
    )

    FetchContent_MakeAvailable(imgui rlimgui)

    add_subdirectory(Levye/Modules/ImGui)

    target_link_libraries(
        LevyeKit

        PRIVATE
            LevyeImGui
    )

    target_compile_definitions(
        LevyeKit

        PUBLIC
            LEVYE_WITH_IMGUI=1
    )
endif()

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

if(LEVYE_WITH_IMGUI)
    target_link_libraries(
        Game

        PRIVATE
            LevyeDearImGui
    )

    target_compile_definitions(
        Game

        PRIVATE
            LEVYE_WITH_IMGUI=1
    )
endif()

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

# ------------------------------------------------------------
# Sandbox runtime dependency
# ------------------------------------------------------------

# Sandbox loads Game dynamically rather than linking against it.
#
# Ensure that building the Sandbox target also builds the game
# module, preventing the executable from loading an outdated
# Game library during development.
add_dependencies(Sandbox Game)

set_target_properties(
    Sandbox

    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}/Targets/$<CONFIG>/bin"
)

if(LEVYE_WITH_SERIALIZATION)
    levye_copy_serialization_runtime(Sandbox)
endif()

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

if(WIN32 AND LEVYE_WITH_IMGUI)
    add_custom_command(
        TARGET Sandbox
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:LevyeDearImGui>
            $<TARGET_FILE_DIR:Sandbox>
        COMMENT "Copying Dear ImGui runtime DLL to Sandbox output directory"
        VERBATIM
    )

    add_custom_command(
        TARGET Game
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:LevyeDearImGui>
            $<TARGET_FILE_DIR:Game>
        COMMENT "Copying Dear ImGui runtime DLL to Game output directory"
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
    Tools/LevyeCLI/AddCommand.cpp

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

option(LEVYE_BUILD_TESTS "Build LevyeKit tests" OFF)

if(LEVYE_BUILD_TESTS)
    enable_testing()

    add_executable(LevyeInputMapTests
        Tests/Input/InputMapTests.cpp
    )

    target_link_libraries(LevyeInputMapTests
        PRIVATE LevyeKit
    )

    add_test(
        NAME InputMap
        COMMAND LevyeInputMapTests
    )
endif()