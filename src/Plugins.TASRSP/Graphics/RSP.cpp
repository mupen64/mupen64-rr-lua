/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "RSP.hpp"

#include "Common.hpp"
#include "DepthBuffer.hpp"
#include "DrawList.hpp"
#include <BuiltinTAS/GraphicsTask.hpp>
#include <algorithm>
#include "N64.hpp"

RSPInfo RSP;
M64RRSpec::PluginInit *graphics_plugin;

void RSP_LoadMatrix(f32 mtx[4][4], u32 address)
{
    constexpr float recip = 1.0 / 65536.0;
    constexpr int offset_lo[] = {2, 0, 6, 4};
    constexpr int offset_hi[] = {0x22, 0x20, 0x26, 0x24};

    uint8_t *base = RDRAM + address;

    for (uint8_t row = 0; row < 4; ++row)
    {
        for (uint8_t col = 0; col < 4; ++col)
        {
            int16_t lo;
            uint16_t hi;
            std::memcpy(&lo, base + offset_lo[col], sizeof(lo));
            std::memcpy(&hi, base + offset_hi[col], sizeof(hi));
            const float result = static_cast<float>(lo) + static_cast<float>(hi) * recip;

            mtx[row][col] = result;
        }

        base += 8;
    }
}

u32 RSP_PeekCommand()
{
    const u32 pc = RSP.PC[RSP.PCi];
    return pc <= RDRAMSize - 4 ? _SHIFTR(*(u32 *)&RDRAM[pc], 24, 8) : G_NOOP;
}

static void update_display_size()
{
    f32 xScale = _FIXED2FLOAT(_SHIFTR(*VI_X_SCALE, 0, 12), 10);

    f32 yScale = _FIXED2FLOAT(_SHIFTR(*VI_Y_SCALE, 0, 12), 10);

    u32 hEnd = _SHIFTR(*VI_H_START, 0, 10);
    u32 hStart = _SHIFTR(*VI_H_START, 16, 10);

    // These are in half-lines, so shift an extra bit
    u32 vEnd = _SHIFTR(*VI_V_START, 1, 9);
    u32 vStart = _SHIFTR(*VI_V_START, 17, 9);

    RSP.width = (hEnd - hStart) * xScale;
    RSP.height = (vEnd - vStart) * yScale * 1.0126582f;

    if (RSP.width == 0.0f) RSP.width = 320.0f;
    if (RSP.height == 0.0f) RSP.height = 240.0f;
}

namespace BuiltinTAS::Graphics
{
void reset()
{
    RSP = {};
    gSP = {};
    gDP = {};
    std::ranges::fill(TMEM, 0);
    gDP.loadTile = &gDP.tiles[7];
    gSP.textureTile[0] = &gDP.tiles[0];
    gSP.textureTile[1] = &gDP.tiles[1];
    DepthBuffer_Destroy();
    DepthBuffer_Init();
    GBI_Destroy();
    GBI_Init();
}

void process_task(M64RRSpec::PluginInit &plugin, SubmitDrawList submit)
{
    graphics_plugin = &plugin;
    update_display_size();
    RSP.has_renderer = submit != nullptr;
    RSP.record_draws = RSP.has_renderer && !plugin.frame_skipped();
    if (RSP.record_draws) begin_draw_list(RSP.width, RSP.height, RSP.DList);

    RSP.PC[0] = *(u32 *)&DMEM[0x0FF0];
    RSP.PCi = 0;

    RSP.halt = FALSE;

    gSP.matrix.stackSize = std::min(32u, *(u32 *)&DMEM[0x0FE4] >> 6);
    gSP.matrix.modelViewi = 0;
    gSP.changed |= CHANGED_MATRIX;

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) gSP.matrix.modelView[0][i][j] = 0.0f;

    gSP.matrix.modelView[0][0][0] = 1.0f;
    gSP.matrix.modelView[0][1][1] = 1.0f;
    gSP.matrix.modelView[0][2][2] = 1.0f;
    gSP.matrix.modelView[0][3][3] = 1.0f;

    u32 uc_start = *(u32 *)&DMEM[0x0FD0];
    u32 uc_dstart = *(u32 *)&DMEM[0x0FD8];
    u32 uc_dsize = *(u32 *)&DMEM[0x0FDC];

    gSPLoadUcodeEx(uc_start, uc_dstart, uc_dsize);

    gDPSetAlphaCompare(G_AC_NONE);
    gDPSetDepthSource(G_ZS_PIXEL);
    gDPSetRenderMode(0, 0);
    gDPSetAlphaDither(G_AD_DISABLE);
    gDPSetColorDither(G_CD_DISABLE);
    gDPSetCombineKey(G_CK_NONE);
    gDPSetTextureConvert(G_TC_FILT);
    gDPSetTextureFilter(G_TF_POINT);
    gDPSetTextureLUT(G_TT_NONE);
    gDPSetTextureLOD(G_TL_TILE);
    gDPSetTextureDetail(G_TD_CLAMP);
    gDPSetTexturePersp(G_TP_PERSP);
    gDPSetCycleType(G_CYC_1CYCLE);
    gDPPipelineMode(G_PM_NPRIMITIVE);

    while (!RSP.halt)
    {
        if (RSP.PC[RSP.PCi] > RDRAMSize - 8)
        {
            DebugMsg(L"ATTEMPTING TO EXECUTE RSP COMMAND AT INVALID RDRAM LOCATION\n");
            break;
        }

        u32 w0 = *(u32 *)&RDRAM[RSP.PC[RSP.PCi]];
        u32 w1 = *(u32 *)&RDRAM[RSP.PC[RSP.PCi] + 4];

        RSP.PC[RSP.PCi] += 8;
        RSP.nextCmd = RSP_PeekCommand();

        GBI.cmd[_SHIFTR(w0, 24, 8)](w0, w1);

        *plugin.rcp_counter += 1;
    }

    if (RSP.record_draws) submit(end_draw_list());
    RSP.DList++;
}
} // namespace BuiltinTAS::Graphics
