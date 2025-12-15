/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for implementing the piano roll frontend.
 */
namespace PianoRoll
{
/**
 * \brief Initializes the subsystem.
 */
void init();

/**
 * Shows the piano roll window.
 */
void show();

/**
 * \brief Gets the HWND of the piano roll window. Might be invalid.
 */
HWND hwnd();
} // namespace PianoRoll
