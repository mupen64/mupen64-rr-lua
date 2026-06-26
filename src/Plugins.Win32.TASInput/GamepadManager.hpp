/*
 * Copyright (c) 2026, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <core_types.h>

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
    uint64_t id;
    std::string name;
    bool connected = true;
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
core_buttons get_input(size_t i);

/**
 * \brief Gets the device registry.
 */
DeviceRegistry &device_registry();

} // namespace GamepadManager
