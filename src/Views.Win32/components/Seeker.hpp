/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

namespace Seeker
{
/**
 * \brief Initializes the seeker subsystem
 */
void init();

/**
 * \brief Shows the seeker dialog
 */
void show();

/**
 * \brief Gets the HWND of the seeker window. Might be invalid.
 */
HWND hwnd();
} // namespace Seeker
