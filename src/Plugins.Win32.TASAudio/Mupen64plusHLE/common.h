/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#define clamp_s16(x) (int16_t)std::clamp((int16_t)(x), (int16_t)INT16_MIN, (int16_t)INT16_MAX)

void SetupMusyX();
void ProcessMusyX_v1();
void ProcessMusyX_v2();
