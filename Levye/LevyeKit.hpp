#pragma once

/**
 * @file LevyeKit.hpp
 * @brief Main public include for the LevyeKit game framework.
 *
 * Including this header provides access to the public game-facing LevyeKit
 * API, including logging, input, screens, time, assets, audio, and the
 * reloadable game module interface.
 */

// Core
#include <Levye/Core/GameAPI.hpp>
#include <Levye/Core/Version.hpp>

// Game-facing systems
#include <Levye/Assets/Assets.hpp>
#include <Levye/Audio/Audio.hpp>
#include <Levye/Debug/Log.hpp>
#include <Levye/Input/Input.hpp>
#include <Levye/Screen/Screen.hpp>
#include <Levye/Time/Time.hpp>

// Optional game-facing modules
#if defined(LEVYE_WITH_SERIALIZATION) && LEVYE_WITH_SERIALIZATION
#include <Levye/Modules/Serialization/Serialization.hpp>
#endif

#if defined(LEVYE_WITH_IMGUI) && LEVYE_WITH_IMGUI
#include <imgui.h>

#include <Levye/Modules/ImGui/ImGui.hpp>
#endif

#if defined(LEVYE_WITH_ANIMATION) && LEVYE_WITH_ANIMATION
#include <Levye/Modules/Animation/Animation.hpp>
#endif

// raylib
#include <raylib.h>
#include <raymath.h>