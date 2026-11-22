/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "RSP.hpp"
#include "DrawList.hpp"
#include "DepthBuffer.hpp"
#include "CRC.hpp"
#include "VI.hpp"

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

void Graphics_Reset()
{
    DepthBuffer_Destroy();
    DepthBuffer_Init();
    GBI_Destroy();
    GBI_Init();
    CRC_BuildTable();
    RSP = {};
    gSP = {};
    gDP = {};
    std::fill_n(TMEM, 512, 0);
    gDP.loadTile = &gDP.tiles[7];
    gSP.textureTile[0] = &gDP.tiles[0];
    gSP.textureTile[1] = &gDP.tiles[1];
    DrawList_Reset();
}

void Graphics_Process(M64RRSpec::PluginInit *plugin)
{
    if (!gDP.loadTile) Graphics_Reset();
    graphics_plugin = plugin;
    VI_UpdateSize();
    DrawList_Begin(draw_sink != nullptr);
    RSP.PC[0] = *(u32 *)&DMEM[0x0FF0];
    RSP.PCi = 0;
    RSP.count = 0;

    RSP.halt = FALSE;
    RSP.frameHas3D = false;

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
        RSP.cmd = _SHIFTR(w0, 24, 8);

        RSP.PC[RSP.PCi] += 8;

        GBI.cmd[RSP.cmd](w0, w1);

        *graphics_plugin->rcp_counter += 1;
    }

    DrawList_End(VI.width, VI.height);
}
