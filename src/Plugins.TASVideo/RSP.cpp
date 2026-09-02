/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "TASVideo.hpp"
#include "OpenGL.hpp"
#include "RSP.hpp"
#include "RDP.hpp"
#include "N64.hpp"
#include "F3D.hpp"
#include "3DMath.hpp"
#include "VI.hpp"
#include "Combiner.hpp"
#include "FrameBuffer.hpp"
#include "DepthBuffer.hpp"
#include "GBI.hpp"
#include <hqx.h>

RSPInfo RSP;

RSPInfo::~RSPInfo()
{
    if (thread)
    {
        RSP_SendMessage(RSPMSG_CLOSE);
        thread->join();
    }
}

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

void RSP_PostMessage(u32 command)
{
    {
        std::lock_guard lock(RSP.mutex);
        RSP.messages.emplace_back(command);
    }
    RSP.msg_available.notify_one();
}

void RSP_SendMessage(u32 command)
{
    std::future<void> completed;
    {
        std::lock_guard lock(RSP.mutex);
        auto &message = RSP.messages.emplace_back(command);
        completed = message.completed.get_future();
    }
    RSP.msg_available.notify_one();
    completed.wait();
}

void RSP_ThreadProc()
{
    hqxInit();

    while (true)
    {
        RSPMessage message;
        {
            std::unique_lock lock(RSP.mutex);
            RSP.msg_available.wait(lock, [] { return !RSP.messages.empty(); });
            message = std::move(RSP.messages.front());
            RSP.messages.pop_front();
        }

        const auto command = message.command;
        if (command == RSPMSG_CLOSE)
        {
            OGL_Stop();
            OGL_DestroyContext();
            message.completed.set_value();
            return;
        }

        switch (command)
        {
        case RSPMSG_START:
            RSP_Init();
            break;
        case RSPMSG_RESTART:
            OGL_Stop();
            OGL_DestroyContext();
            OGL_Start();
            OGL_ResizeWindow();
            break;
        case RSPMSG_PROCESSDLIST:
            RSP_ProcessDList();
            break;
        case RSPMSG_DESTROYTEXTURES:
            Combiner_Destroy();
            FrameBuffer_Destroy();
            TextureCache_Destroy();
            break;
        case RSPMSG_INITTEXTURES:
            FrameBuffer_Init();
            TextureCache_Init();
            Combiner_Init();
            gSP.changed = gDP.changed = 0xFFFFFFFF;
            break;
        case RSPMSG_READPIXELS:
            OGL_ReadPixels();
            break;
        case RSPMSG_BLACKOUT:
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            break;
        default:
            break;
        }
        message.completed.set_value();
    }
}

void RSP_ProcessDList()
{
    OGL.headless = g_plugin->frame_skipped();

    if (!OGL.headless)
    {
        VI_UpdateSize();
        OGL_UpdateScale();

        if (OGL.clear_override)
        {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            const GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
            glDisable(GL_SCISSOR_TEST);
            glClear(GL_COLOR_BUFFER_BIT);
            if (scissor) glEnable(GL_SCISSOR_TEST);
        }
    }

    RSP.PC[0] = *(u32 *)&DMEM[0x0FF0];
    RSP.PCi = 0;
    RSP.count = 0;

    RSP.halt = FALSE;
    RSP.busy = TRUE;

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

    if (uc_start != RSP.uc_start || uc_dstart != RSP.uc_dstart) gSPLoadUcodeEx(uc_start, uc_dstart, uc_dsize);

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

    const auto cmds = OGL.headless ? GBI.cmd_headless : GBI.cmd;
    while (!RSP.halt)
    {
        if (RSP.PC[RSP.PCi] + 8 > RDRAMSize)
        {
            DebugMsg(L"ATTEMPTING TO EXECUTE RSP COMMAND AT INVALID RDRAM LOCATION\n");
            break;
        }

        u32 w0 = *(u32 *)&RDRAM[RSP.PC[RSP.PCi]];
        u32 w1 = *(u32 *)&RDRAM[RSP.PC[RSP.PCi] + 4];
        RSP.cmd = _SHIFTR(w0, 24, 8);

        RSP.PC[RSP.PCi] += 8;
        RSP.nextCmd = _SHIFTR(*(u32 *)&RDRAM[RSP.PC[RSP.PCi]], 24, 8);

        cmds[RSP.cmd](w0, w1);

        *g_plugin->rcp_counter += 1;
    }

    RSP.busy = FALSE;
    RSP.DList++;
    gSP.changed |= CHANGED_COLORBUFFER;
}

void RSP_Init()
{
    RSP.DList = 0;
    RSP.uc_start = RSP.uc_dstart = 0;

    gDP.loadTile = &gDP.tiles[7];
    gSP.textureTile[0] = &gDP.tiles[0];
    gSP.textureTile[1] = &gDP.tiles[1];
    DepthBuffer_Init();
    GBI_Init();
    OGL_Start();
}
