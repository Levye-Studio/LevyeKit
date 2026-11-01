# LevyeKit Animation module.

add_library(LevyeAnimation STATIC
  AnimationService.cpp
  AnimationResource.cpp
  AnimationStateMachine.cpp
  EditablePose.cpp
    IK/TwoBoneIK.cpp
    IK/MultiChainIK.cpp
    IK/EndEffectorOrientation.cpp
    Stabilization/BodyStabilization.cpp
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
