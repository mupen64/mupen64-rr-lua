/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module that provides parity checking functionality based on a running hash of sync-determining emulator
 * state.
 */
namespace ParityChecker
{
void begin(int32_t interval);
void on_sample(int32_t sample);
void end();
bool active();
} // namespace ParityChecker
