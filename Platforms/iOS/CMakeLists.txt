cmake_minimum_required(VERSION 3.20)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    message(FATAL_ERROR "Configure iOS with -DCMAKE_SYSTEM_NAME=iOS.")
endif()

project(LevyeIOS LANGUAGES C CXX OBJC)

foreach(LEVYE_REQUIRED IN ITEMS LEVYE_ROOT LEVYE_PROJECT_ROOT)
    if(NOT DEFINED ${LEVYE_REQUIRED} OR NOT IS_DIRECTORY "${${LEVYE_REQUIRED}}")
        message(FATAL_ERROR "${LEVYE_REQUIRED} must point to a directory.")
    endif()
endforeach()

set(LEVYE_WITH_IOS ON CACHE BOOL "" FORCE)
set(LEVYE_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(LEVYE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(LEVYE_IOS_BUILD_NAME "${CMAKE_BINARY_DIR}" NAME)
set(LEVYE_IOS_TARGET_DIRECTORY
    "${LEVYE_PROJECT_ROOT}/Targets/${LEVYE_IOS_BUILD_NAME}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${LEVYE_IOS_TARGET_DIRECTORY}")
foreach(LEVYE_IOS_CONFIG Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${LEVYE_IOS_CONFIG}" LEVYE_IOS_CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${LEVYE_IOS_CONFIG_UPPER}
        "${LEVYE_IOS_TARGET_DIRECTORY}")
endforeach()

add_subdirectory("${LEVYE_ROOT}" framework)

set(LEVYE_GAME_SOURCES "${LEVYE_PROJECT_ROOT}/Source/Game.cpp")
include("${LEVYE_PROJECT_ROOT}/Source/CMakeLists.txt" OPTIONAL)

add_executable(levye_ios MACOSX_BUNDLE
    "${CMAKE_CURRENT_SOURCE_DIR}/iOSMain.cpp" ${LEVYE_GAME_SOURCES})
target_include_directories(levye_ios PRIVATE "${LEVYE_PROJECT_ROOT}/Source")
target_link_libraries(levye_ios PRIVATE LevyeKit)
get_target_property(LEVYE_ANGLE_FRAMEWORKS raylib LEVYE_ANGLE_FRAMEWORKS)
if(NOT LEVYE_ANGLE_FRAMEWORKS)
    message(FATAL_ERROR "iOS raylib target has no ANGLE frameworks to embed.")
endif()

set(LEVYE_IOS_BUNDLE_ID "com.levye.game" CACHE STRING "iOS bundle identifier")
set(LEVYE_IOS_DISPLAY_NAME "Levye Game" CACHE STRING "iOS display name")
set(LEVYE_IOS_OUTPUT_NAME "${LEVYE_IOS_DISPLAY_NAME}" CACHE STRING "iOS app bundle filename")
set(LEVYE_IOS_VERSION_NAME "1.0.0" CACHE STRING "iOS version name")
set(LEVYE_IOS_VERSION_CODE "1" CACHE STRING "iOS version code")
set(LEVYE_IOS_DEPLOYMENT_TARGET "15.0" CACHE STRING "Minimum iOS version")
set(LEVYE_IOS_ORIENTATION "all" CACHE STRING "Supported iOS orientation")
if(LEVYE_IOS_ORIENTATION STREQUAL "portrait")
    set(LEVYE_IOS_ORIENTATIONS "<string>UIInterfaceOrientationPortrait</string>")
elseif(LEVYE_IOS_ORIENTATION STREQUAL "landscape")
    set(LEVYE_IOS_ORIENTATIONS "<string>UIInterfaceOrientationLandscapeLeft</string><string>UIInterfaceOrientationLandscapeRight</string>")
elseif(LEVYE_IOS_ORIENTATION STREQUAL "all")
    set(LEVYE_IOS_ORIENTATIONS "<string>UIInterfaceOrientationPortrait</string><string>UIInterfaceOrientationLandscapeLeft</string><string>UIInterfaceOrientationLandscapeRight</string>")
else()
    message(FATAL_ERROR "Invalid iOS orientation: ${LEVYE_IOS_ORIENTATION}")
endif()
set(LEVYE_IOS_DISPLAY_NAME_XML "${LEVYE_IOS_DISPLAY_NAME}")
string(REPLACE "&" "&amp;" LEVYE_IOS_DISPLAY_NAME_XML "${LEVYE_IOS_DISPLAY_NAME_XML}")
string(REPLACE "<" "&lt;" LEVYE_IOS_DISPLAY_NAME_XML "${LEVYE_IOS_DISPLAY_NAME_XML}")
string(REPLACE ">" "&gt;" LEVYE_IOS_DISPLAY_NAME_XML "${LEVYE_IOS_DISPLAY_NAME_XML}")
set(LEVYE_IOS_EXECUTABLE_NAME "${LEVYE_IOS_OUTPUT_NAME}")
set(LEVYE_IOS_ASSET_DIRECTORY "Assets" CACHE STRING
    "Asset directory relative to the project root, matching levye.project")

if("${LEVYE_IOS_ASSET_DIRECTORY}" STREQUAL "" OR
   "${LEVYE_IOS_ASSET_DIRECTORY}" STREQUAL "." OR
   IS_ABSOLUTE "${LEVYE_IOS_ASSET_DIRECTORY}" OR
   "${LEVYE_IOS_ASSET_DIRECTORY}" MATCHES "(^|/)\.\.(/|$)")
    message(FATAL_ERROR "iOS asset directory must be project-relative.")
endif()
if(NOT IS_DIRECTORY "${LEVYE_PROJECT_ROOT}/${LEVYE_IOS_ASSET_DIRECTORY}")
    message(FATAL_ERROR "iOS asset directory does not exist: ${LEVYE_IOS_ASSET_DIRECTORY}")
endif()

set_target_properties(levye_ios PROPERTIES
    OUTPUT_NAME "${LEVYE_IOS_OUTPUT_NAME}"
    # Use the embedded ANGLE pair at runtime, never the dependency cache.
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "@executable_path/Frameworks"
    MACOSX_BUNDLE_GUI_IDENTIFIER "${LEVYE_IOS_BUNDLE_ID}"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${LEVYE_IOS_BUNDLE_ID}"
    XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "${LEVYE_IOS_DEPLOYMENT_TARGET}"
    XCODE_EMBED_FRAMEWORKS "${LEVYE_ANGLE_FRAMEWORKS}"
    XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY YES
    XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY YES
    )

# On iOS the executable and resource directory live in the .app bundle.
# Copy the manifest and its matching asset path there for ProjectLoader.
add_custom_command(TARGET levye_ios POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        "-DLEVYE_IOS_PROJECT_ROOT=${LEVYE_PROJECT_ROOT}"
        "-DLEVYE_IOS_ASSET_DIRECTORY=${LEVYE_IOS_ASSET_DIRECTORY}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/CopyResources.cmake"
    VERBATIM)
