# LevyeKit Animation module.

add_library(LevyeAnimation STATIC
  AnimationService.cpp
  AnimationResource.cpp
  AnimationStateMachine.cpp
    IK/TwoBoneIK.cpp
)

target_include_directories(LevyeAnimation
    PUBLIC
        "${PROJECT_SOURCE_DIR}"
)

target_compile_features(LevyeAnimation
    PUBLIC
        cxx_std_20
)

target_link_libraries(LevyeAnimation
    PUBLIC
        raylib
)
