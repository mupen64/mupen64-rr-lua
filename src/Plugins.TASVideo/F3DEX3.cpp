/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "TASVideo.hpp"
#include "F3DEX2.hpp"
#include "F3DEX3.hpp"
#include "N64.hpp"
#include "RSP.hpp"
#include "gSP.hpp"
#include "GBI.hpp"
#include "OpenGL.hpp"

void F3DEX3_Mtx(u32 w0, u32 w1)
{
    gSPMatrix(w1, _SHIFTR(w0, 0, 8) ^ G_MTX_PUSH ^ G_MTX_LOAD);
}

void F3DEX3_MoveMem(u32 w0, u32 w1)
{
    switch (_SHIFTR(w0, 0, 8))
    {
    case F3DEX3_MV_VIEWPORT:
        gSPViewport(w1);

        gSP.viewport.vscale[1] = -gSP.viewport.vscale[1];
        gSP.viewport.vscale[2] /= 32.0f;
        gSP.viewport.vtrans[2] /= 32.0f;
        gSP.viewport.y = gSP.viewport.vtrans[1] - gSP.viewport.vscale[1];
        gSP.viewport.height = gSP.viewport.vscale[1] * 2;
        gSP.viewport.nearz = gSP.viewport.vtrans[2] - gSP.viewport.vscale[2];
        gSP.viewport.farz = gSP.viewport.vtrans[2] + gSP.viewport.vscale[2];
        break;
    case F3DEX3_MV_LIGHT: {
        const u32 offset = _SHIFTR(w0, 8, 8) << 3;
        const u32 length = (_SHIFTR(w0, 19, 5) + 1) << 3;

        for (u32 pos = offset; pos < offset + length; pos += 16)
        {
            if (pos >= 0x10) gSPLight(w1 + (pos - offset), (pos - 0x10) / 0x10 + 1);
        }
        break;
    }
    }
}

void F3DEX3_MoveWord(u32 w0, u32 w1)
{
    switch (_SHIFTR(w0, 16, 8))
    {
    case F3DEX3_MW_FX:
        break;
    case G_MW_NUMLIGHT:
        gSPNumLights(_SHIFTR(w1, 4, 7));
        break;
    case G_MW_LIGHTCOL:
        if ((_SHIFTR(w0, 0, 16) & 0xF) == 0) gSPLightColor(_SHIFTR(w0, 0, 16) / 0x10 + 1, w1);
        break;
    default:
        F3DEX2_MoveWord(w0, w1);
        break;
    }
}

void F3DEX3_TriSnake(u32 w0, u32 w1)
{
    u32 words[2] = {w0, w1};
    u32 pos = 3;
    u32 a = _SHIFTR(w0, 17, 7);
    u32 b = _SHIFTR(w0, 9, 7);
    u32 c = 0;

    for (u32 n = 0; n < 1024; n++)
    {
        if (pos == 8)
        {
            if (RSP.PC[RSP.PCi] + 8 > RDRAMSize) break;

            words[0] = *(u32 *)&RDRAM[RSP.PC[RSP.PCi]];
            words[1] = *(u32 *)&RDRAM[RSP.PC[RSP.PCi] + 4];
            RSP.PC[RSP.PCi] += 8;
            pos = 0;
        }

        const u32 shift = 24 - 8 * (pos & 3);
        const u32 byte = _SHIFTR(words[pos >> 2], shift, 8);
        pos++;

        if (byte & 1)
            c = a;
        else
            b = a;
        a = _SHIFTR(byte, 1, 6);

        if (!OGL.headless) gSPTriangle(a, b, c, 0);

        if (byte & 0x80) break;
    }

    if (OGL.numTriangles > 0) OGL_DrawTriangles();
}

void F3DEX3_RelSegment(u32 w0, u32 w1)
{
    gSPSegment(_SHIFTR(w0, 0, 12) >> 2, RSP_SegmentToPhysical(w1));
}

void F3DEX3_Init()
{
    F3DEX2_Init();

    GBI.cmd[F3DEX2_MTX] = F3DEX3_Mtx;
    GBI.cmd[F3DEX2_MOVEMEM] = F3DEX3_MoveMem;
    GBI.cmd[F3DEX2_MOVEWORD] = F3DEX3_MoveWord;
    GBI.cmd[F3DEX3_TRISNAKE] = F3DEX3_TriSnake;
    GBI.cmd[F3DEX3_RELSEGMENT] = F3DEX3_RelSegment;
}
