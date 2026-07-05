/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

namespace CLI
{
/**
 * \brief Initializes the CLI
 */
void init();

/**
 * Gets the speed mode desired by the CLI.
 */
CoreSpeedMode desired_speed_mode();
} // namespace CLI
