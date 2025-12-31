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
 * \brief Notifies of an SDL event.
 * \brief e The SDL event.
 */
void on_sdl_event(const SDL_Event &e);

/**
 * \brief Gets the current gamepad input state.
 * \param i The controller index.
 */
core_buttons get_input(size_t i);
} // namespace GamepadManager
