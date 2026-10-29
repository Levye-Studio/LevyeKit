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

# Make the missing header available to yaml-cpp 0.8.0.
# GCC 16 no longer provides uint16_t through incidental includes.
set(YAML_CPP_COMPAT_DIR "${CMAKE_CURRENT_BINARY_DIR}/yaml-cpp-compat")
file(MAKE_DIRECTORY "${YAML_CPP_COMPAT_DIR}")

file(
    WRITE "${YAML_CPP_COMPAT_DIR}/cstdint_compat.hpp"
    "#include <cstdint>\n"
)

# Compatibility with newer CMake versions.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_MakeAvailable(yaml_cpp)

if(TARGET yaml-cpp)
    target_compile_options(
        yaml-cpp
        PRIVATE
        "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-include${YAML_CPP_COMPAT_DIR}/cstdint_compat.hpp>"
    )
endif()

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
