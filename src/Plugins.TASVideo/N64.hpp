/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "TASVideo.hpp"
#include "Types.hpp"

#define MI_INTR_DP 0x20 // Bit 5: DP intr

#define MI_INTR (&g_plugin->mi_register->mi_intr_reg)

#define DPC_START (&g_plugin->dpc_register->dpc_start)
#define DPC_END (&g_plugin->dpc_register->dpc_end)
#define DPC_CURRENT (&g_plugin->dpc_register->dpc_current)
#define DPC_STATUS (&g_plugin->dpc_register->dpc_status)
#define DPC_CLOCK (&g_plugin->dpc_register->dpc_clock)
#define DPC_BUFBUSY (&g_plugin->dpc_register->dpc_bufbusy)
#define DPC_PIPEBUSY (&g_plugin->dpc_register->dpc_pipebusy)
#define DPC_TMEM (&g_plugin->dpc_register->dpc_tmem)

#define VI_STATUS (&g_plugin->vi_register->vi_status)
#define VI_ORIGIN (&g_plugin->vi_register->vi_origin)
#define VI_WIDTH (&g_plugin->vi_register->vi_width)
#define VI_INTR (&g_plugin->vi_register->vi_v_intr)
#define VI_V_CURRENT_LINE (&g_plugin->vi_register->vi_current)
#define VI_TIMING (&g_plugin->vi_register->vi_burst)
#define VI_V_SYNC (&g_plugin->vi_register->vi_v_sync)
#define VI_H_SYNC (&g_plugin->vi_register->vi_h_sync)
#define VI_LEAP (&g_plugin->vi_register->vi_leap)
#define VI_H_START (&g_plugin->vi_register->vi_h_start)
#define VI_V_START (&g_plugin->vi_register->vi_v_start)
#define VI_V_BURST (&g_plugin->vi_register->vi_v_burst)
#define VI_X_SCALE (&g_plugin->vi_register->vi_x_scale)
#define VI_Y_SCALE (&g_plugin->vi_register->vi_y_scale)

#define DMEM (g_plugin->dmem)
#define IMEM (g_plugin->imem)
#define RDRAM (g_plugin->rdram)

inline u64 TMEM[512];
inline u32 RDRAMSize = 0x800000;
