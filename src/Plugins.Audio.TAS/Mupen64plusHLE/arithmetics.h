/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <cstdint>

static int16_t clamp_s16(int32_t x)
{
    x = x < INT16_MIN ? INT16_MIN : x;
    x = x > INT16_MAX ? INT16_MAX : x;

    return (int16_t)x;
}
