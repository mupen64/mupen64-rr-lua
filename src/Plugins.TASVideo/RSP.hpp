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
#include <condition_variable>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <thread>

#define RSPMSG_CLOSE 0
#define RSPMSG_START 1
#define RSPMSG_PROCESSDLIST 3
#define RSPMSG_DESTROYTEXTURES 5
#define RSPMSG_INITTEXTURES 6
#define RSPMSG_READPIXELS 7
#define RSPMSG_RESTART 8
#define RSPMSG_BLACKOUT 9

struct RSPMessage
{
    u32 command;
    std::promise<void> completed;
};

struct RSPInfo
{
    std::unique_ptr<std::jthread> thread;
    std::mutex mutex;
    std::condition_variable msg_available;
    std::deque<RSPMessage> messages;

    u32 PC[18], PCi, busy, halt, close, DList, uc_start, uc_dstart, cmd, nextCmd, count;
};

extern RSPInfo RSP;

#define RSP_SegmentToPhysical(segaddr) ((gSP.segment[(segaddr >> 24) & 0x0F] + (segaddr & 0x00FFFFFF)) & 0x00FFFFFF)

void RSP_Init();
void RSP_ProcessDList();
void RSP_ThreadProc();
void RSP_PostMessage(u32 message);
void RSP_SendMessage(u32 message);
void RSP_LoadMatrix(f32 mtx[4][4], u32 address);
