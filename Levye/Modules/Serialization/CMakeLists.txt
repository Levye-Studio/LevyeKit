# ---------------------------------------------------------
# Optional serialization module
# ---------------------------------------------------------

include(FetchContent)

FetchContent_Declare(
    yaml_cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0
)

# We don't need yaml-cpp's development utilities.
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)

# Compatibility with newer CMake versions.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_MakeAvailable(yaml_cpp)

add_library(
    LevyeSerialization STATIC

    SerializationService.cpp
    SerializationBridge.cpp    
)

target_include_directories(
    LevyeSerialization

    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(
    LevyeSerialization

    PRIVATE
        yaml-cpp::yaml-cpp
)

target_compile_features(
    LevyeSerialization

    PUBLIC
        cxx_std_20
)