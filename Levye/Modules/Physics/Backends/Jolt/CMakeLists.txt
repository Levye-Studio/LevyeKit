
# ---------------------------------------------------------
# Jolt Physics dependency
# ---------------------------------------------------------

include(FetchContent)

set(
    TARGET_UNIT_TESTS OFF
    CACHE BOOL "" FORCE
)

set(
    TARGET_HELLO_WORLD OFF
    CACHE BOOL "" FORCE
)

set(
    TARGET_SAMPLES OFF
    CACHE BOOL "" FORCE
)

set(
    TARGET_VIEWER OFF
    CACHE BOOL "" FORCE
)

FetchContent_Declare(
    JoltPhysics

    GIT_REPOSITORY
        https://github.com/jrouwe/JoltPhysics.git

    GIT_TAG
        v5.6.0

    SOURCE_SUBDIR
        Build
)

FetchContent_MakeAvailable(JoltPhysics)

# ---------------------------------------------------------
# Jolt backend implementation
# ---------------------------------------------------------

target_sources(
    LevyePhysics

    PRIVATE
        JoltBackend.cpp
)

target_link_libraries(
    LevyePhysics

    PRIVATE
        Jolt
)

target_compile_definitions(
    LevyePhysics

    PUBLIC
        LEVYE_PHYSICS_JOLT=1
)
