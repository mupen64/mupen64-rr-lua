/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "TASVideo.hpp"
#include "Types.hpp"

#define MI_INTR_DP 0x20 // Bit 5: DP intr

#define MI_INTR (&BuiltinTAS::TASVideo::g_plugin->mi_register->mi_intr_reg)

#define DPC_START (&BuiltinTAS::TASVideo::g_plugin->dpc_register->dpc_start)
#define DPC_END (&BuiltinTAS::TASVideo::g_plugin->dpc_register->dpc_end)
#define DPC_CURRENT (&BuiltinTAS::TASVideo::g_plugin->dpc_register->dpc_current)
#define DPC_STATUS (&BuiltinTAS::TASVideo::g_plugin->dpc_register->dpc_status)
#define DPC_CLOCK (&BuiltinTAS::TASVideo::g_plugin->dpc_register->dpc_clock)
#define DPC_BUFBUSY (&BuiltinTAS::TASVideo::g_plugin->dpc_register->dpc_bufbusy)
#define DPC_PIPEBUSY (&BuiltinTAS::TASVideo::g_plugin->dpc_register->dpc_pipebusy)
#define DPC_TMEM (&BuiltinTAS::TASVideo::g_plugin->dpc_register->dpc_tmem)

#define VI_STATUS (&BuiltinTAS::TASVideo::g_plugin->vi_register->vi_status)
#define VI_ORIGIN (&BuiltinTAS::TASVideo::g_plugin->vi_register->vi_origin)
#define VI_WIDTH (&BuiltinTAS::TASVideo::g_plugin->vi_register->vi_width)
#define VI_INTR (&BuiltinTAS::TASVideo::g_plugin->vi_register->vi_v_intr)
#define VI_V_CURRENT_LINE (&BuiltinTAS::TASVideo::g_plugin->vi_register->vi_current)
#define VI_TIMING (&BuiltinTAS::TASVideo::g_plugin->vi_register->vi_burst)
#define VI_V_SYNC (&BuiltinTAS::TASVideo::g_plugin->vi_register->vi_v_sync)
#define VI_H_SYNC (&BuiltinTAS::TASVideo::g_plugin->vi_register->vi_h_sync)
#define VI_LEAP (&BuiltinTAS::TASVideo::g_plugin->vi_register->vi_leap)
#define VI_H_START (&BuiltinTAS::TASVideo::g_plugin->vi_register->vi_h_start)
#define VI_V_START (&BuiltinTAS::TASVideo::g_plugin->vi_register->vi_v_start)
#define VI_V_BURST (&BuiltinTAS::TASVideo::g_plugin->vi_register->vi_v_burst)
#define VI_X_SCALE (&BuiltinTAS::TASVideo::g_plugin->vi_register->vi_x_scale)
#define VI_Y_SCALE (&BuiltinTAS::TASVideo::g_plugin->vi_register->vi_y_scale)

#define DMEM (BuiltinTAS::TASVideo::g_plugin->dmem)
#define IMEM (BuiltinTAS::TASVideo::g_plugin->imem)
#define RDRAM (BuiltinTAS::TASVideo::g_plugin->rdram)

inline u64 TMEM[512];
inline u32 RDRAMSize = 0x800000;
