# Keep the optional-module cache in sync with the generated project's manifest.
# This file is included by the current template and by LevyeKit itself for
# projects generated with older templates.
include_guard(GLOBAL)

set(_levye_project_manifest "${CMAKE_SOURCE_DIR}/levye.project")
if(NOT EXISTS "${_levye_project_manifest}")
    return()
endif()

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${_levye_project_manifest}")

set(_levye_serialization OFF)
set(_levye_imgui OFF)
set(_levye_animation OFF)
set(_levye_in_modules OFF)
file(STRINGS "${_levye_project_manifest}" _levye_project_lines)
foreach(_levye_line IN LISTS _levye_project_lines)
    if(_levye_line MATCHES "^[ \t]*\\[([^]]+)\\][ \t]*(#.*)?$")
        if(CMAKE_MATCH_1 STREQUAL "modules")
            set(_levye_in_modules ON)
        else()
            set(_levye_in_modules OFF)
        endif()
    elseif(_levye_in_modules AND
           _levye_line MATCHES "^[ \t]*(serialization|imgui|animation)[ \t]*=[ \t]*(true|false)[ \t]*(#.*)?$")
        if(CMAKE_MATCH_2 STREQUAL "true")
            set(_levye_${CMAKE_MATCH_1} ON)
        else()
            set(_levye_${CMAKE_MATCH_1} OFF)
        endif()
    endif()
endforeach()

set(LEVYE_WITH_SERIALIZATION "${_levye_serialization}" CACHE BOOL
    "Enable LevyeKit serialization from levye.project" FORCE)
set(LEVYE_WITH_IMGUI "${_levye_imgui}" CACHE BOOL
    "Enable LevyeKit ImGui from levye.project" FORCE)
set(LEVYE_WITH_ANIMATION "${_levye_animation}" CACHE BOOL
    "Enable LevyeKit animation from levye.project" FORCE)
