cmake_minimum_required(VERSION 3.20)

# Also synchronize projects created with older templates. In a standalone
# framework build there is no root levye.project, so command-line options win.
if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR AND
   EXISTS "${CMAKE_SOURCE_DIR}/levye.project")
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/LevyeProjectModules.cmake")
    # Older generated CMake files did not give Game every module definition.
    # Apply selected module guards to parent-directory targets as well.
    foreach(LEVYE_MODULE IN ITEMS SERIALIZATION IMGUI ANIMATION PHYSICS)
        if(LEVYE_WITH_${LEVYE_MODULE})
            set_property(DIRECTORY "${CMAKE_SOURCE_DIR}" APPEND PROPERTY
                COMPILE_DEFINITIONS "LEVYE_WITH_${LEVYE_MODULE}=1")
        endif()
    endforeach()
endif()

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

option(
    LEVYE_WITH_ANIMATION
    "Enable the LevyeKit Animation module"
    OFF
)

option(
    LEVYE_WITH_PHYSICS
    "Enable the LevyeKit Physics module"
    OFF
)

option(
    LEVYE_WITH_ANDROID
    "Enable LevyeKit Android support"
    OFF
)

option(
    LEVYE_WITH_IOS
    "Enable LevyeKit iOS support"
    OFF
)

set(
    LEVYE_PHYSICS_BACKEND
    "JOLT"
    CACHE STRING "Physics backend"
)

set_property(
    CACHE LEVYE_PHYSICS_BACKEND
    PROPERTY STRINGS JOLT
)

project(
    LevyeKit
    VERSION 0.5.0
    LANGUAGES CXX
)

if(LEVYE_WITH_ANDROID AND NOT ANDROID)
    message(FATAL_ERROR "LEVYE_WITH_ANDROID requires an Android toolchain.")
endif()

if(ANDROID AND NOT LEVYE_WITH_ANDROID)
    message(FATAL_ERROR "Android toolchains require LEVYE_WITH_ANDROID=ON.")
endif()

if(LEVYE_WITH_IOS)
    enable_language(OBJC)
    enable_language(OBJCXX)
    if(NOT APPLE OR NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
        message(FATAL_ERROR "LEVYE_WITH_IOS requires an iOS toolchain.")
    endif()
    foreach(LEVYE_MODULE IN ITEMS SERIALIZATION IMGUI ANIMATION PHYSICS)
        if(LEVYE_WITH_${LEVYE_MODULE})
            message(FATAL_ERROR "iOS does not yet support ${LEVYE_MODULE}.")
        endif()
    endforeach()
endif()

if(LEVYE_WITH_ANDROID)
    foreach(LEVYE_MODULE IN ITEMS SERIALIZATION IMGUI ANIMATION PHYSICS)
        if(LEVYE_WITH_${LEVYE_MODULE})
            message(FATAL_ERROR "Android does not yet support ${LEVYE_MODULE}.")
        endif()
    endforeach()
endif()

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

# ---------------------------------------------------------------------------
# Build-specific output directories
# ---------------------------------------------------------------------------
# -B Build/build-animation -> Targets/build-animation
# -B Build/build-no-animation -> Targets/build-no-animation
get_filename_component(
    LEVYE_BUILD_NAME
    "${CMAKE_BINARY_DIR}"
    NAME
)

set(
    LEVYE_OUTPUT_DIR
    "${CMAKE_SOURCE_DIR}/Targets/${LEVYE_BUILD_NAME}"
)

if((EMSCRIPTEN OR LEVYE_WITH_IOS) AND DEFINED LEVYE_PROJECT_ROOT)
    set(LEVYE_OUTPUT_DIR "${LEVYE_PROJECT_ROOT}/Targets/${LEVYE_BUILD_NAME}")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${LEVYE_OUTPUT_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${LEVYE_OUTPUT_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${LEVYE_OUTPUT_DIR}/lib")

# Multi-config generators otherwise append /Debug, /Release, etc.
foreach(LEVYE_CONFIG Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${LEVYE_CONFIG}" LEVYE_CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${LEVYE_CONFIG_UPPER} "${LEVYE_OUTPUT_DIR}/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${LEVYE_CONFIG_UPPER} "${LEVYE_OUTPUT_DIR}/lib")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${LEVYE_CONFIG_UPPER} "${LEVYE_OUTPUT_DIR}/lib")
endforeach()

message(STATUS "LevyeKit output directory: ${LEVYE_OUTPUT_DIR}")


include(FetchContent)

# The host and reloadable game module must use the same raylib instance.
if(LEVYE_WITH_IOS)
    set(PLATFORM iOS CACHE STRING "raylib platform" FORCE)
    # raylib-iOS release/6.2 has a native backend but no iOS branch in
    # LibraryConfigurations.cmake. Supply its compile definitions directly.
    set(PLATFORM_CPP PLATFORM_IOS)
    set(GRAPHICS GRAPHICS_API_OPENGL_ES2)
    set(OPENGL_VERSION "OFF" CACHE STRING "" FORCE)
    set(USE_EXTERNAL_GLFW OFF CACHE STRING "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
elseif(EMSCRIPTEN)
    set(PLATFORM Web CACHE STRING "raylib platform" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    foreach(module SERIALIZATION IMGUI ANIMATION PHYSICS)
        if(LEVYE_WITH_${module})
            message(FATAL_ERROR "Web does not yet support ${module}.")
        endif()
    endforeach()
else()
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
endif()
set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)

set(LEVYE_RAYLIB_TAG "6.2")
set(LEVYE_RAYLIB_REPOSITORY "https://github.com/raysan5/raylib.git")
set(LEVYE_RAYLIB_CACHE_VERSION "6.2")
set(LEVYE_RAYLIB_VERIFY_REF "refs/tags/6.2^{commit}")

if(LEVYE_WITH_IOS)
    set(LEVYE_RAYLIB_TAG "release/6.2")
    set(LEVYE_RAYLIB_REPOSITORY "https://github.com/ghera/raylib-iOS.git")
    set(LEVYE_RAYLIB_CACHE_VERSION "ios-6.2")
    set(LEVYE_RAYLIB_VERIFY_REF "refs/remotes/origin/release/6.2^{commit}")
endif()

# A caller-provided source override takes priority.
if(NOT FETCHCONTENT_SOURCE_DIR_RAYLIB)
    if(DEFINED ENV{LEVYE_DEPENDENCY_CACHE}
       AND NOT "$ENV{LEVYE_DEPENDENCY_CACHE}" STREQUAL "")
        set(LEVYE_CACHE_ROOT "$ENV{LEVYE_DEPENDENCY_CACHE}")
    elseif(DEFINED ENV{XDG_CACHE_HOME}
           AND NOT "$ENV{XDG_CACHE_HOME}" STREQUAL "")
        set(LEVYE_CACHE_ROOT "$ENV{XDG_CACHE_HOME}/levye/dependencies")
    elseif(DEFINED ENV{HOME})
        set(LEVYE_CACHE_ROOT "$ENV{HOME}/.cache/levye/dependencies")
    else()
        message(FATAL_ERROR
            "Set LEVYE_DEPENDENCY_CACHE to a writable cache directory.")
    endif()

    set(LEVYE_RAYLIB_CACHE
        "${LEVYE_CACHE_ROOT}/raylib/${LEVYE_RAYLIB_CACHE_VERSION}")
    set(LEVYE_RAYLIB_SOURCE "${LEVYE_RAYLIB_CACHE}/source")

    file(MAKE_DIRECTORY "${LEVYE_RAYLIB_CACHE}")
    file(LOCK "${LEVYE_RAYLIB_CACHE}/cache.lock"
         GUARD PROCESS TIMEOUT 300)

    find_package(Git REQUIRED)

    set(LEVYE_RAYLIB_VALID FALSE)

    if(EXISTS "${LEVYE_RAYLIB_SOURCE}/.git")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${LEVYE_RAYLIB_SOURCE}"
                    rev-parse HEAD
            OUTPUT_VARIABLE LEVYE_RAYLIB_HEAD
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE LEVYE_HEAD_RESULT
            ERROR_QUIET
        )

        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${LEVYE_RAYLIB_SOURCE}"
                    rev-parse "${LEVYE_RAYLIB_VERIFY_REF}"
            OUTPUT_VARIABLE LEVYE_RAYLIB_TAG_COMMIT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE LEVYE_TAG_RESULT
            ERROR_QUIET
        )

        if(LEVYE_HEAD_RESULT EQUAL 0
           AND LEVYE_TAG_RESULT EQUAL 0
           AND LEVYE_RAYLIB_HEAD STREQUAL LEVYE_RAYLIB_TAG_COMMIT
           AND EXISTS "${LEVYE_RAYLIB_SOURCE}/src/raylib.h")
            set(LEVYE_RAYLIB_VALID TRUE)
        endif()
    endif()

    if(NOT LEVYE_RAYLIB_VALID)
        if(FETCHCONTENT_FULLY_DISCONNECTED
           OR FETCHCONTENT_UPDATES_DISCONNECTED
           OR FETCHCONTENT_UPDATES_DISCONNECTED_RAYLIB)
            message(FATAL_ERROR
                "raylib ${LEVYE_RAYLIB_TAG} is missing from the shared cache: "
                "${LEVYE_RAYLIB_SOURCE}. Offline mode is enabled.")
        endif()

        if(EXISTS "${LEVYE_RAYLIB_SOURCE}")
            message(FATAL_ERROR
                "Invalid raylib cache at ${LEVYE_RAYLIB_SOURCE}. "
                "Move it aside before retrying.")
        endif()

        set(LEVYE_RAYLIB_TEMP
            "${LEVYE_RAYLIB_CACHE}/source-incomplete")

        file(REMOVE_RECURSE "${LEVYE_RAYLIB_TEMP}")

        message(STATUS "Downloading raylib ${LEVYE_RAYLIB_TAG} to shared cache")

        execute_process(
            COMMAND "${GIT_EXECUTABLE}" clone
                    --depth 1
                    --branch "${LEVYE_RAYLIB_TAG}"
                    "${LEVYE_RAYLIB_REPOSITORY}"
                    "${LEVYE_RAYLIB_TEMP}"
            RESULT_VARIABLE LEVYE_CLONE_RESULT
        )

        if(NOT LEVYE_CLONE_RESULT EQUAL 0)
            file(REMOVE_RECURSE "${LEVYE_RAYLIB_TEMP}")
            message(FATAL_ERROR "Failed to download raylib ${LEVYE_RAYLIB_TAG}.")
        endif()

        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${LEVYE_RAYLIB_TEMP}"
                    rev-parse HEAD
            OUTPUT_VARIABLE LEVYE_TEMP_HEAD
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE LEVYE_TEMP_HEAD_RESULT
        )

        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${LEVYE_RAYLIB_TEMP}"
                    rev-parse "${LEVYE_RAYLIB_VERIFY_REF}"
            OUTPUT_VARIABLE LEVYE_TEMP_TAG
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE LEVYE_TEMP_TAG_RESULT
        )

        if(NOT LEVYE_TEMP_HEAD_RESULT EQUAL 0
           OR NOT LEVYE_TEMP_TAG_RESULT EQUAL 0
           OR NOT LEVYE_TEMP_HEAD STREQUAL LEVYE_TEMP_TAG
           OR NOT EXISTS "${LEVYE_RAYLIB_TEMP}/src/raylib.h")
            file(REMOVE_RECURSE "${LEVYE_RAYLIB_TEMP}")
            message(FATAL_ERROR "raylib ${LEVYE_RAYLIB_TAG} verification failed.")
        endif()

        file(RENAME
            "${LEVYE_RAYLIB_TEMP}"
            "${LEVYE_RAYLIB_SOURCE}")
    else()
        message(STATUS
            "Reusing shared raylib source: ${LEVYE_RAYLIB_SOURCE}")
    endif()

    set(FETCHCONTENT_SOURCE_DIR_RAYLIB
        "${LEVYE_RAYLIB_SOURCE}")
endif()

# Preserve the existing source-override version check.
set(LEVYE_RAYLIB_HEADER
    "${FETCHCONTENT_SOURCE_DIR_RAYLIB}/src/raylib.h")

if(NOT EXISTS "${LEVYE_RAYLIB_HEADER}")
    message(FATAL_ERROR
        "Local raylib source is missing src/raylib.h: "
        "${FETCHCONTENT_SOURCE_DIR_RAYLIB}")
endif()

file(STRINGS "${LEVYE_RAYLIB_HEADER}" LEVYE_RAYLIB_VERSION_LINE
     REGEX "^#define RAYLIB_VERSION[ \t]+\"6\\.2\"$")

if(NOT LEVYE_RAYLIB_VERSION_LINE)
    message(FATAL_ERROR
        "Local raylib source must be version 6.2: "
        "${FETCHCONTENT_SOURCE_DIR_RAYLIB}")
endif()

FetchContent_Declare(
    raylib
    GIT_REPOSITORY "${LEVYE_RAYLIB_REPOSITORY}"
    GIT_TAG "${LEVYE_RAYLIB_TAG}"
)

if(LEVYE_WITH_IOS)
    # The raylib-iOS release/6.2 fork implements PLATFORM_IOS but its
    # CMakeOptions.txt rejects PLATFORM=iOS. Build its sources directly,
    # without editing the shared, version-pinned dependency cache.
    set(raylib_SOURCE_DIR "${FETCHCONTENT_SOURCE_DIR_RAYLIB}")
    set(LEVYE_IOS_RAYLIB_SOURCES
        rcore.c rshapes.c rtext.c rtextures.c raudio.c rmodels.c)
    list(TRANSFORM LEVYE_IOS_RAYLIB_SOURCES
        PREPEND "${raylib_SOURCE_DIR}/src/")

    add_library(raylib STATIC ${LEVYE_IOS_RAYLIB_SOURCES})

    target_include_directories(raylib PUBLIC "${raylib_SOURCE_DIR}/src")

    target_compile_definitions(raylib PUBLIC
        PLATFORM_IOS
        GRAPHICS_API_OPENGL_ES3
        GL_GLEXT_PROTOTYPES
        )

    # Use the ANGLE frameworks bundled with raylib-iOS 6.2.
    set(LEVYE_ANGLE_ROOT "${raylib_SOURCE_DIR}/deps/ANGLE")

    if(CMAKE_OSX_SYSROOT MATCHES "simulator")
        set(LEVYE_ANGLE_SLICE "ios-arm64_x86_64-simulator")
    else()
        set(LEVYE_ANGLE_SLICE "ios-arm64")
    endif()

    set(LEVYE_ANGLE_FRAMEWORKS
        "${LEVYE_ANGLE_ROOT}/libEGL.xcframework/${LEVYE_ANGLE_SLICE}/libEGL.framework"
        "${LEVYE_ANGLE_ROOT}/libGLESv2.xcframework/${LEVYE_ANGLE_SLICE}/libGLESv2.framework"
    )
    set_property(TARGET raylib PROPERTY LEVYE_ANGLE_FRAMEWORKS
        "${LEVYE_ANGLE_FRAMEWORKS}")

    foreach(framework IN LISTS LEVYE_ANGLE_FRAMEWORKS)
        if(NOT EXISTS "${framework}")
            message(FATAL_ERROR "Missing ANGLE framework: ${framework}")
        endif()
    endforeach()

    # Makes <libEGL/...> and <libGLESv2/...> headers discoverable.
    foreach(framework IN LISTS LEVYE_ANGLE_FRAMEWORKS)
        get_filename_component(framework_dir "${framework}" DIRECTORY)
        target_compile_options(raylib PUBLIC "-F${framework_dir}")
        target_link_options(raylib PUBLIC "-F${framework_dir}")
    endforeach()

    # Resolve fork header order and Apple type collisions in rcore.c only.
    set_source_files_properties("${raylib_SOURCE_DIR}/src/rcore.c" PROPERTIES
        COMPILE_OPTIONS "-include;${CMAKE_CURRENT_SOURCE_DIR}/Platforms/iOS/RaylibCompat.h")

    target_link_libraries(raylib PUBLIC ${LEVYE_ANGLE_FRAMEWORKS})

    # rcore.c includes the Objective-C UIKit backend implementation.
    set_source_files_properties(
        "${raylib_SOURCE_DIR}/src/rcore.c"
        PROPERTIES LANGUAGE OBJC)

    # miniaudio includes Apple Objective-C framework headers on iOS.
    set_source_files_properties(
        "${raylib_SOURCE_DIR}/src/raudio.c"
        PROPERTIES LANGUAGE OBJC
    )
    foreach(LEVYE_IOS_FRAMEWORK IN ITEMS UIKit Foundation QuartzCore
            CoreGraphics AudioToolbox AVFoundation CoreAudio)
        find_library(LEVYE_IOS_${LEVYE_IOS_FRAMEWORK}
            NAMES ${LEVYE_IOS_FRAMEWORK} REQUIRED)
        target_link_libraries(raylib PUBLIC
            "${LEVYE_IOS_${LEVYE_IOS_FRAMEWORK}}")
    endforeach()
else()
    FetchContent_MakeAvailable(raylib)
endif()

if(ANDROID)
    # raylib hides GetAndroidApp in shared builds. The host needs the native
    # activity to query cutout insets without changing cached raylib sources.
    set_property(SOURCE "${raylib_SOURCE_DIR}/src/rcore.c"
        TARGET_DIRECTORY raylib APPEND PROPERTY COMPILE_OPTIONS
        "-fvisibility=default")
endif()

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

    Levye/Display/DisplayLayout.cpp
    Levye/Display/DisplayManager.cpp

    Levye/Input/InputMap.cpp
    Levye/Input/TouchManager.cpp
    Levye/Screen/ScreenManager.cpp
    Levye/Assets/TextureManager.cpp
    Levye/Audio/AudioManager.cpp
    Levye/Graphics/ShaderManager.cpp
    Levye/Graphics/FontManager.cpp

    Levye/IO/FileWatcher.cpp
    Levye/Assets/AssetPath.cpp
    Levye/Platform/ExecutablePath.cpp
    Levye/Platform/RuntimePaths.cpp
    Levye/Platform/SharedLibrary.cpp
    Levye/Time/TimeSystem.cpp
    Levye/Project/ProjectConfig.cpp
    Levye/Project/ProjectLoader.cpp

    Levye/Debug/Logger.cpp
)

if(LEVYE_WITH_IOS)
    target_sources(LevyeKit PRIVATE Platforms/iOS/SafeArea.mm)
elseif(ANDROID)
    target_sources(LevyeKit PRIVATE Platforms/Android/SafeArea.cpp)
    target_include_directories(LevyeKit PRIVATE
        "${ANDROID_NDK}/sources/android/native_app_glue")
endif()

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

if(LEVYE_WITH_ANIMATION)
    add_subdirectory(
        Levye/Modules/Animation
    )

    target_link_libraries(
        LevyeKit

        PRIVATE
            LevyeAnimation
    )

    target_compile_definitions(
        LevyeKit

        PUBLIC
            LEVYE_WITH_ANIMATION=1
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

if(LEVYE_WITH_PHYSICS)
    add_subdirectory(
        Levye/Modules/Physics
    )

    target_link_libraries(
        LevyeKit

        PRIVATE
            LevyePhysics
    )

    target_compile_definitions(
        LevyeKit

        PUBLIC
            LEVYE_WITH_PHYSICS=1
    )
endif()

if(LEVYE_BUILD_EXAMPLES AND LEVYE_WITH_ANDROID)
    # raylib's Android NativeActivity calls main() from libmain.so.
    add_library(SandboxAndroid SHARED
        Examples/Sandbox/AndroidMain.cpp
        Examples/Sandbox/Game/Game.cpp
        Examples/Sandbox/Game/Scenes/Audio/AudioScene.cpp
        Examples/Sandbox/Game/Scenes/Core/CoreScene.cpp
        Examples/Sandbox/Game/Scenes/Input/InputScene.cpp
        Examples/Sandbox/Game/Scenes/Menu/MenuScene.cpp
        Examples/Sandbox/Game/Scenes/Serialization/SerializationScene.cpp)

    target_include_directories(SandboxAndroid PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(SandboxAndroid PRIVATE LevyeKit)

    set_target_properties(SandboxAndroid PROPERTIES OUTPUT_NAME main)

    # Current Android emulator images use 16 KiB pages.
    target_link_options(SandboxAndroid PRIVATE "LINKER:-z,max-page-size=16384")

    target_link_options(raylib PRIVATE "LINKER:-z,max-page-size=16384")
elseif(LEVYE_BUILD_EXAMPLES)

# ---------------------------------------------------------------------------
# Sandbox Game Module
# ---------------------------------------------------------------------------

add_library(
    Game SHARED

    Examples/Sandbox/Game/Game.cpp
    Examples/Sandbox/Game/Scenes/Audio/AudioScene.cpp
    Examples/Sandbox/Game/Scenes/Core/CoreScene.cpp
    Examples/Sandbox/Game/Scenes/Input/InputScene.cpp
    Examples/Sandbox/Game/Scenes/Menu/MenuScene.cpp
    Examples/Sandbox/Game/Scenes/Serialization/SerializationScene.cpp
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
    target_sources(Game PRIVATE Examples/Sandbox/Game/Scenes/ImGui/ImGuiScene.cpp)
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

if(LEVYE_WITH_PHYSICS)
    target_sources(
        Game

        PRIVATE
            Examples/Sandbox/Game/Scenes/Physics/PhysicsScene.cpp
    )

    target_compile_definitions(
        Game

        PRIVATE
            LEVYE_WITH_PHYSICS=1
    )
endif()

if(LEVYE_WITH_ANIMATION)
    target_sources(Game PRIVATE Examples/Sandbox/Game/Scenes/Animation/AnimationScene.cpp
        Examples/Sandbox/Game/Scenes/Animation/LocomotionScene.cpp
        Examples/Sandbox/Game/Scenes/Animation/OrientationScene.cpp)
    target_link_libraries(Game PRIVATE LevyeAnimation)
    target_compile_definitions(
        Game
        PRIVATE
            LEVYE_WITH_ANIMATION=1
    )
endif()

set_target_properties(
    Game

    PROPERTIES
        PREFIX ""
        OUTPUT_NAME "Game"

        LIBRARY_OUTPUT_DIRECTORY
            "${LEVYE_OUTPUT_DIR}/lib"

        RUNTIME_OUTPUT_DIRECTORY
            "${LEVYE_OUTPUT_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${LEVYE_OUTPUT_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${LEVYE_OUTPUT_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${LEVYE_OUTPUT_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${LEVYE_OUTPUT_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${LEVYE_OUTPUT_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${LEVYE_OUTPUT_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${LEVYE_OUTPUT_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${LEVYE_OUTPUT_DIR}/lib"
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
            "${LEVYE_OUTPUT_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${LEVYE_OUTPUT_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${LEVYE_OUTPUT_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${LEVYE_OUTPUT_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${LEVYE_OUTPUT_DIR}/bin"
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
    Tools/LevyeCLI/ProjectCommand.cpp
    Tools/LevyeCLI/PlatformPreparation.cpp
    Tools/LevyeCLI/ProjectLocator.cpp
    Tools/LevyeCLI/BuildCommand.cpp
    Tools/LevyeCLI/WebBuild.cpp
    Tools/LevyeCLI/RunCommand.cpp
    Tools/LevyeCLI/CleanCommand.cpp
    Tools/LevyeCLI/AddCommand.cpp
    Tools/LevyeCLI/GenerateCommand.cpp
    Tools/LevyeCLI/ProjectCMakeMigration.cpp
    Tools/LevyeCLI/FileTransaction.cpp
    Tools/LevyeCLI/ProjectIDE.cpp
    Tools/LevyeCLI/UpgradeCommand.cpp
    Tools/LevyeCLI/ProjectModuleMigration.cpp
    Tools/LevyeCLI/ConvertCommand.cpp
    Tools/LevyeCLI/ProjectAnalyzer.cpp

    Levye/Project/ProjectConfig.cpp
    Levye/Project/ProjectLoader.cpp
    Levye/Platform/ExecutablePath.cpp

    Tools/LevyeCLI/ExportCommand.cpp
    Tools/LevyeCLI/Export/IOSExporter.cpp
    Tools/LevyeCLI/Export/AndroidExporter.cpp
    Tools/LevyeCLI/Export/DesktopExporter.cpp
    Tools/LevyeCLI/Export/WebExporter.cpp
    Tools/LevyeCLI/Export/AndroidSigning.cpp
)

set_target_properties(
    levye
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY
        "${LEVYE_OUTPUT_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${LEVYE_OUTPUT_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${LEVYE_OUTPUT_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${LEVYE_OUTPUT_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${LEVYE_OUTPUT_DIR}/bin"
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
