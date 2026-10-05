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

message(
    STATUS
    "LevyeKit serialization: ${LEVYE_WITH_SERIALIZATION}"
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
