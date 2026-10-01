cmake_minimum_required(VERSION 3.20)

project(
    {{PROJECT_TARGET}}
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

if(NOT CMAKE_BUILD_TYPE)
    set(
        CMAKE_BUILD_TYPE
        Debug
        CACHE STRING
        "Build type"
        FORCE
    )
endif()

set(
    LEVYE_TARGET_DIRECTORY
    "${CMAKE_SOURCE_DIR}/Targets/${CMAKE_BUILD_TYPE}"
)

set_target_properties(
    Game
    PROPERTIES
        PREFIX ""
        LIBRARY_OUTPUT_DIRECTORY
        "${LEVYE_TARGET_DIRECTORY}/lib"
)

set_target_properties(
    {{PROJECT_TARGET}}
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY
        "${LEVYE_TARGET_DIRECTORY}/bin"
)