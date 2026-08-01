/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "Types.hpp"

struct VIInfo
{
    u32 width, height;
    u32 lastOrigin;
};

extern VIInfo VI;

void VI_UpdateSize();
void VI_UpdateScreen();