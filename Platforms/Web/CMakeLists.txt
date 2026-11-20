cmake_minimum_required(VERSION 3.20)

project(LevyeWeb LANGUAGES C CXX)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "Use emcmake to configure Web exports.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(LEVYE_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(LEVYE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory("${LEVYE_ROOT}" framework)

set(LEVYE_GAME_SOURCES "${LEVYE_PROJECT_ROOT}/Source/Game.cpp")

include("${LEVYE_PROJECT_ROOT}/Source/CMakeLists.txt" OPTIONAL)

add_executable(levye_web WebMain.cpp ${LEVYE_GAME_SOURCES})

target_include_directories(levye_web PRIVATE "${LEVYE_PROJECT_ROOT}/Source")

target_link_libraries(levye_web PRIVATE LevyeKit)

set_target_properties(levye_web PROPERTIES OUTPUT_NAME index SUFFIX ".html"
    RUNTIME_OUTPUT_DIRECTORY "${LEVYE_WEB_OUTPUT}")

target_link_options(levye_web PRIVATE
    "-sALLOW_MEMORY_GROWTH=1" "-sFORCE_FILESYSTEM=1"
    "--preload-file" "${LEVYE_WEB_PAYLOAD}@/"
    "-sUSE_GLFW=3"
    "--shell-file" "${CMAKE_CURRENT_SOURCE_DIR}/shell.html"
    "-sASYNCIFY=1"
    "-sASSERTIONS=2"
    )

set_property(TARGET levye_web APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/shell.html")
# Asset modifications must trigger relinking even when C++ is unchanged.
file(GLOB_RECURSE payload_files CONFIGURE_DEPENDS "${LEVYE_WEB_PAYLOAD}/*")
set_property(TARGET levye_web APPEND PROPERTY LINK_DEPENDS "${payload_files}")
