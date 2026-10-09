cmake_minimum_required(VERSION 3.20)

project(
    {{PROJECT_TARGET}}
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Single-configuration generators need a default; VS chooses at build time.
if(WIN32 AND NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# LevyeKit checkout used to create this project.
set(
    LEVYE_ROOT
    "{{LEVYE_ROOT}}"
)

if(NOT EXISTS "${LEVYE_ROOT}/CMakeLists.txt")
    message(
        FATAL_ERROR
        "LevyeKit could not be found at: ${LEVYE_ROOT}"
    )
endif()

# Generated projects only need the LevyeKit framework itself.
# Examples and development tools belong to the framework repository.
set(
    LEVYE_BUILD_EXAMPLES
    OFF
    CACHE BOOL ""
    FORCE
)

set(
    LEVYE_BUILD_TOOLS
    OFF
    CACHE BOOL ""
    FORCE
)

# ---------------------------------------------------------
# Optional LevyeKit modules
# ---------------------------------------------------------

set(LEVYE_WITH_SERIALIZATION OFF)
set(LEVYE_WITH_IMGUI OFF)

# Reconfigure when levye add changes the project modules.
set_property(
    DIRECTORY
    APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/levye.project"
)

# Read the serialization setting from levye.project.
file(
    STRINGS
    "${CMAKE_CURRENT_SOURCE_DIR}/levye.project"
    LEVYE_SERIALIZATION_SETTING
    REGEX "^[ \t]*serialization[ \t]*=[ \t]*(true|false)[ \t]*$"
)

if(LEVYE_SERIALIZATION_SETTING)
    if(LEVYE_SERIALIZATION_SETTING MATCHES "=[ \t]*true[ \t]*$")
        set(LEVYE_WITH_SERIALIZATION ON)
    endif()
endif()

# Read the ImGui setting from levye.project.
file(
    STRINGS
    "${CMAKE_CURRENT_SOURCE_DIR}/levye.project"
    LEVYE_IMGUI_SETTING
    REGEX "^[ \t]*imgui[ \t]*=[ \t]*(true|false)[ \t]*$"
)

if(LEVYE_IMGUI_SETTING)
    if(LEVYE_IMGUI_SETTING MATCHES "=[ \t]*true[ \t]*$")
        set(LEVYE_WITH_IMGUI ON)
    endif()
endif()

message(
    STATUS
    "LevyeKit serialization: ${LEVYE_WITH_SERIALIZATION}"
)

message(
    STATUS
    "LevyeKit ImGui: ${LEVYE_WITH_IMGUI}"
)

add_subdirectory(
    "${LEVYE_ROOT}"
    "${CMAKE_BINARY_DIR}/LevyeKit"
)

# ---------------------------------------------------------
# Reloadable game module
# ---------------------------------------------------------

add_library(
    Game
    SHARED
    Source/Game.cpp
)

target_include_directories(
    Game
    PRIVATE
        "${LEVYE_ROOT}"
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

# ---------------------------------------------------------
# Game host executable
# ---------------------------------------------------------

add_executable(
    {{PROJECT_TARGET}}
    Source/Main.cpp
)

target_include_directories(
    {{PROJECT_TARGET}}
    PRIVATE
        "${LEVYE_ROOT}"
)

target_link_libraries(
    {{PROJECT_TARGET}}
    PRIVATE
        LevyeKit
)

# ---------------------------------------------------------
# Output directories
# ---------------------------------------------------------

if(WIN32)
    set(LEVYE_TARGET_DIRECTORY "${CMAKE_SOURCE_DIR}/Targets/$<CONFIG>")
else()
    # Preserve the existing macOS/Linux configuration and output layout.
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    endif()
    set(LEVYE_TARGET_DIRECTORY "${CMAKE_SOURCE_DIR}/Targets/${CMAKE_BUILD_TYPE}")
endif()

set_target_properties(
    Game
    PROPERTIES
        PREFIX ""
        LIBRARY_OUTPUT_DIRECTORY
        "${LEVYE_TARGET_DIRECTORY}/lib"
)

if(WIN32)
    set_target_properties(Game PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${LEVYE_TARGET_DIRECTORY}/lib"
        ARCHIVE_OUTPUT_DIRECTORY "${LEVYE_TARGET_DIRECTORY}/lib")
endif()

set_target_properties(
    {{PROJECT_TARGET}}
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY
        "${LEVYE_TARGET_DIRECTORY}/bin"
)

# Deploy raylib's shared library beside the game executable.
# Windows requires runtime DLL dependencies to be discoverable
# when the executable starts.
if(WIN32)
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:raylib>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying raylib runtime DLL"
        VERBATIM
    )
endif()

if(WIN32 AND LEVYE_WITH_IMGUI)
    # The host needs the Dear ImGui DLL when the application starts.
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:LevyeDearImGui>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying Dear ImGui runtime DLL"
        VERBATIM
    )

    # Game.dll is loaded dynamically from Targets/<Config>/lib.
    # Keep the dependency there too so the game module can resolve it
    # independently of the host executable's location.
    add_custom_command(
        TARGET Game
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:LevyeDearImGui>
            $<TARGET_FILE_DIR:Game>
        COMMENT "Copying Dear ImGui runtime DLL for Game"
        VERBATIM
    )
endif()