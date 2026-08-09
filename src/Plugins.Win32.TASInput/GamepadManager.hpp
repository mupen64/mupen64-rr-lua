/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "Main.hpp"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>

/**
 * \brief Provides gamepad-related functionality.
 */
namespace GamepadManager
{

enum class InputDeviceType
{
    Keyboard,
    Gamepad,
};

struct InputDevice
{
    InputDeviceType type;
    std::optional<SDL_GUID> guid;
    std::string name;
};

struct DeviceRegistry
{
    std::vector<InputDevice> devices;
};

/**
 * \brief Notifies of an SDL event.
 * \brief e The SDL event.
 */
void on_sdl_event(const SDL_Event &e);

/**
 * \brief Gets the current gamepad input state.
 * \param i The controller index.
 */
CoreButtons get_input(size_t i);

/**
 * \brief Updates the currently selected gamepad. Should be called after `new_config.preferred_device_id` is changed.
 */
void update_current_gamepad();

/**
 * \brief Gets the device registry.
 */
DeviceRegistry &device_registry();

} // namespace GamepadManager
