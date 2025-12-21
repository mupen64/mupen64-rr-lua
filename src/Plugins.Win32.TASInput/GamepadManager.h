/*
 * Copyright (c) 2025, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief Provides gamepad-related functionality.
 */
namespace GamepadManager
{
    /**
     * \brief Initializes the subsystem.
     */
    void init();

    /**
     * \brief Polls for gamepad events.
     */
    void poll_events();

    /**
     * \brief Gets the current gamepad input state.
     */
    core_buttons get_input();
} // namespace GamepadManager
