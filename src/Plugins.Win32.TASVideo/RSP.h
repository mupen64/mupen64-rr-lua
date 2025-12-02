#ifndef RSP_H
#define RSP_H

#include <windows.h>
#include "N64.h"
#include "GBI.h"
#include "gSP.h"
#include "Types.h"

#define RSPMSG_CLOSE 0
#define RSPMSG_START 1
#define RSPMSG_UPDATESCREEN 2
#define RSPMSG_PROCESSDLIST 3
#define RSPMSG_CAPTURESCREEN 4
#define RSPMSG_DESTROYTEXTURES 5
#define RSPMSG_INITTEXTURES 6
#define RSPMSG_READPIXELS 7

using RSPInfo = struct
{
    HANDLE thread;

    u32 PC[18], PCi, busy, halt, close, DList, uc_start, uc_dstart, cmd, nextCmd, count;

    // Events for thread messages, see defines at the top, or RSP_Thread
    HANDLE threadMsg[8];
    // Event to notify main process that the RSP is finished with what it was doing
    HANDLE threadFinished;
};

extern RSPInfo RSP;

#define RSP_SegmentToPhysical(segaddr) ((gSP.segment[(segaddr >> 24) & 0x0F] + (segaddr & 0x00FFFFFF)) & 0x00FFFFFF)

void RSP_Init();
void RSP_ProcessDList();
DWORD WINAPI RSP_ThreadProc(LPVOID lpParameter);
void RSP_LoadMatrix(f32 mtx[4][4], u32 address);

#endif
