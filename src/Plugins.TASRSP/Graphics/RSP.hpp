/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "Types.hpp"
#include "N64.hpp"
#include "GBI.hpp"
#include "gSP.hpp"
struct RSPInfo
{
    u32 PC[18], PCi, halt, DList, nextCmd;
    u32 width, height;
    bool has_renderer;
    bool record_draws;
};

extern RSPInfo RSP;

#define RSP_SegmentToPhysical(segaddr) ((gSP.segment[(segaddr >> 24) & 0x0F] + (segaddr & 0x00FFFFFF)) & 0x00FFFFFF)

void RSP_LoadMatrix(f32 mtx[4][4], u32 address);
u32 RSP_PeekCommand();
