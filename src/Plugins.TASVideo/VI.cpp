/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "TASVideo.hpp"
#include "VI.hpp"
#include "OpenGL.hpp"
#include "N64.hpp"
#include "gSP.hpp"
#include "gDP.hpp"
#include "RSP.hpp"
#include "FrameBuffer.hpp"

VIInfo VI;

void VI_UpdateSize()
{
    f32 xScale = _FIXED2FLOAT(_SHIFTR(*VI_X_SCALE, 0, 12), 10);
    f32 xOffset = _FIXED2FLOAT(_SHIFTR(*VI_X_SCALE, 16, 12), 10);

    f32 yScale = _FIXED2FLOAT(_SHIFTR(*VI_Y_SCALE, 0, 12), 10);
    f32 yOffset = _FIXED2FLOAT(_SHIFTR(*VI_Y_SCALE, 16, 12), 10);

    u32 hEnd = _SHIFTR(*VI_H_START, 0, 10);
    u32 hStart = _SHIFTR(*VI_H_START, 16, 10);

    // These are in half-lines, so shift an extra bit
    u32 vEnd = _SHIFTR(*VI_V_START, 1, 9);
    u32 vStart = _SHIFTR(*VI_V_START, 17, 9);

    VI.width = (hEnd - hStart) * xScale;
    VI.height = (vEnd - vStart) * yScale * 1.0126582f;

    if (VI.width == 0.0f) VI.width = 320.0f;
    if (VI.height == 0.0f) VI.height = 240.0f;
}

void VI_UpdateScreen()
{
    glFinish();

    if (gSP.changed & CHANGED_COLORBUFFER)
    {
        gSP.changed &= ~CHANGED_COLORBUFFER;
    }

    glFinish();
}
