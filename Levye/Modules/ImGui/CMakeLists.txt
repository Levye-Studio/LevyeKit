# Dear ImGui core.
add_library(LevyeDearImGui SHARED
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)

target_include_directories(LevyeDearImGui
    PUBLIC
        ${imgui_SOURCE_DIR}
)

target_compile_features(LevyeDearImGui PUBLIC cxx_std_20)

# raylib <-> Dear ImGui integration.
add_library(LevyeRlImGui STATIC
    ${rlimgui_SOURCE_DIR}/rlImGui.cpp
)

target_include_directories(LevyeRlImGui
    PUBLIC
        ${rlimgui_SOURCE_DIR}
)

target_compile_features(LevyeRlImGui PUBLIC cxx_std_20)

target_link_libraries(LevyeRlImGui
    PUBLIC
        LevyeDearImGui
        raylib
)

# LevyeKit's host-owned ImGui module.
add_library(LevyeImGui STATIC
    ImGuiService.cpp
)

target_include_directories(LevyeImGui
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_compile_features(LevyeImGui PUBLIC cxx_std_20)

target_link_libraries(LevyeImGui
    PUBLIC
        LevyeRlImGui
)