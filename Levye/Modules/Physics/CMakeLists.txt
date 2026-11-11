
cmake_minimum_required(VERSION 3.20)

# ---------------------------------------------------------
# Levye Physics module
# ---------------------------------------------------------

add_library(LevyePhysics STATIC
    ContactPairRegistry.cpp
    PhysicsService.cpp
    PhysicsBridge.cpp
)

target_include_directories(LevyePhysics
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_compile_features(
    LevyePhysics

    PUBLIC
        cxx_std_20
)

target_link_libraries(
    LevyePhysics

    PUBLIC
        raylib
)

set_target_properties(
    LevyePhysics

    PROPERTIES
        POSITION_INDEPENDENT_CODE ON
)

# ---------------------------------------------------------
# Physics backend selection
# ---------------------------------------------------------

if(LEVYE_PHYSICS_BACKEND STREQUAL "JOLT")
    add_subdirectory(Backends/Jolt)

elseif(LEVYE_PHYSICS_BACKEND STREQUAL "BULLET")
    add_subdirectory(Backends/Bullet)

elseif(LEVYE_PHYSICS_BACKEND STREQUAL "PHYSX")
    add_subdirectory(Backends/PhysX)

else()
    message(FATAL_ERROR
        "Unsupported physics backend: ${LEVYE_PHYSICS_BACKEND}"
    )
endif()