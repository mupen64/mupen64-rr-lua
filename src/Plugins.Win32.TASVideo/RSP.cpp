#include "stdafx.h"
#include "glN64.h"
#include "OpenGL.h"
#include "RSP.h"
#include "RDP.h"
#include "N64.h"
#include "F3D.h"
#include "3DMath.h"
#include "VI.h"
#include "Combiner.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "GBI.h"
#include <hqx.h>

RSPInfo RSP;

void RSP_LoadMatrix(f32 mtx[4][4], u32 address)
{
    constexpr float recip = 1.0 / 65536.0;
    constexpr int offset_lo[] = {2, 0, 6, 4};
    constexpr int offset_hi[] = {0x22, 0x20, 0x26, 0x24};

    uint8_t* base = RDRAM + address;

    for (uint8_t row = 0; row < 4; ++row)
    {
        for (uint8_t col = 0; col < 4; ++col)
        {
            const int16_t lo = *reinterpret_cast<int16_t*>(base + offset_lo[col]);
            const uint16_t hi = *reinterpret_cast<uint16_t*>(base + offset_hi[col]);
            const float result = static_cast<float>(lo) + static_cast<float>(hi) * recip;
            
            mtx[row][col] = result;
        }

        base += 8;
    }
}

DWORD WINAPI RSP_ThreadProc(LPVOID)
{
    hqxInit();

    SetEvent(RSP.threadFinished);

    while (TRUE)
    {
        switch (WaitForMultipleObjects(std::size(RSP.threadMsg), RSP.threadMsg, FALSE, INFINITE))
        {
        case WAIT_OBJECT_0 + RSPMSG_CLOSE:
            OGL_Stop();
            break;
        case WAIT_OBJECT_0 + RSPMSG_START:
            RSP_Init();
            break;
        case WAIT_OBJECT_0 + RSPMSG_PROCESSDLIST:
            RSP_ProcessDList();
            break;
        case WAIT_OBJECT_0 + RSPMSG_UPDATESCREEN:
            VI_UpdateScreen();
            break;
        case WAIT_OBJECT_0 + RSPMSG_DESTROYTEXTURES:
            Combiner_Destroy();
            FrameBuffer_Destroy();
            TextureCache_Destroy();
            break;
        case WAIT_OBJECT_0 + RSPMSG_INITTEXTURES:
            FrameBuffer_Init();
            TextureCache_Init();
            Combiner_Init();
            gSP.changed = gDP.changed = 0xFFFFFFFF;
            break;
        case WAIT_OBJECT_0 + RSPMSG_CAPTURESCREEN:
            OGL_SaveScreenshot();
            break;
        case WAIT_OBJECT_0 + RSPMSG_READPIXELS:
            OGL_ReadPixels();
            break;
        }
        SetEvent(RSP.threadFinished);
    }

    RSP.thread = NULL;
    return 0;
}

void RSP_ProcessDList()
{
    VI_UpdateSize();
    OGL_UpdateScale();

    if (OGL.clear_override)
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    RSP.PC[0] = *(u32*)&DMEM[0x0FF0];
    RSP.PCi = 0;
    RSP.count = 0;

    RSP.halt = FALSE;
    RSP.busy = TRUE;

    gSP.matrix.stackSize = min(32, *(u32*)&DMEM[0x0FE4] >> 6);
    gSP.matrix.modelViewi = 0;
    gSP.changed |= CHANGED_MATRIX;

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            gSP.matrix.modelView[0][i][j] = 0.0f;

    gSP.matrix.modelView[0][0][0] = 1.0f;
    gSP.matrix.modelView[0][1][1] = 1.0f;
    gSP.matrix.modelView[0][2][2] = 1.0f;
    gSP.matrix.modelView[0][3][3] = 1.0f;

    u32 uc_start = *(u32*)&DMEM[0x0FD0];
    u32 uc_dstart = *(u32*)&DMEM[0x0FD8];
    u32 uc_dsize = *(u32*)&DMEM[0x0FDC];

    if (uc_start != RSP.uc_start || uc_dstart != RSP.uc_dstart)
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
        if (RSP.PC[RSP.PCi] + 8 > RDRAMSize)
        {
            DebugMsg(L"ATTEMPTING TO EXECUTE RSP COMMAND AT INVALID RDRAM LOCATION\n");
            break;
        }

        u32 w0 = *(u32*)&RDRAM[RSP.PC[RSP.PCi]];
        u32 w1 = *(u32*)&RDRAM[RSP.PC[RSP.PCi] + 4];
        RSP.cmd = _SHIFTR(w0, 24, 8);

        DebugMsg(L"0x%08lX: CMD=0x%02lX W0=0x%08lX W1=0x%08lX\n", RSP.PC[RSP.PCi], _SHIFTR(w0, 24, 8), w0, w1);

        RSP.PC[RSP.PCi] += 8;
        RSP.nextCmd = _SHIFTR(*(u32*)&RDRAM[RSP.PC[RSP.PCi]], 24, 8);

        GBI.cmd[RSP.cmd](w0, w1);
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
