/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <CommonPCH.h>
#include <core_api.h>
#include <Views.Win32/ViewPlugin.h>
#include "Types.h"
#include <windows.h>
#include <cassert>
#include <commctrl.h>

#define DEBUG_OUTPUT

extern core_audio_info AudioInfo;

void HLEStart();
void ChangeABI(int type); /* type 0 = SafeMode */

#define AI_STATUS_FIFO_FULL 0x80000000 /* Bit 31: full */
#define AI_STATUS_DMA_BUSY 0x40000000 /* Bit 30: busy */
#define MI_INTR_AI 0x04 /* Bit 2: AI intr */
#define AI_CONTROL_DMA_ON 0x01

#define PLUGIN_VERSION "1.0.0"

#ifdef _M_X64
#define PLUGIN_ARCH " x64"
#else
#define PLUGIN_ARCH " "
#endif

#ifdef _DEBUG
#define PLUGIN_TARGET " Debug"
#else
#define PLUGIN_TARGET " "
#endif

#define PLUGIN_FULL_NAME "TAS Audio " PLUGIN_VERSION PLUGIN_ARCH PLUGIN_TARGET
