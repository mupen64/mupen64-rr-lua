/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
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
void start(int32_t interval);
void stop();
void on_sample(int32_t sample);
bool active();
} // namespace ParityChecker
