/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Core/Plugin.hpp>
extern M64RRSpec::PluginInit *graphics_plugin;
#include "Types.hpp"

#define MI_INTR_DP 0x20 // Bit 5: DP intr

#define MI_INTR (&graphics_plugin->mi_register->mi_intr_reg)

#define DPC_START (&graphics_plugin->dpc_register->dpc_start)
#define DPC_END (&graphics_plugin->dpc_register->dpc_end)
#define DPC_CURRENT (&graphics_plugin->dpc_register->dpc_current)
#define DPC_STATUS (&graphics_plugin->dpc_register->dpc_status)
#define DPC_CLOCK (&graphics_plugin->dpc_register->dpc_clock)
#define DPC_BUFBUSY (&graphics_plugin->dpc_register->dpc_bufbusy)
#define DPC_PIPEBUSY (&graphics_plugin->dpc_register->dpc_pipebusy)
#define DPC_TMEM (&graphics_plugin->dpc_register->dpc_tmem)

#define VI_STATUS (&graphics_plugin->vi_register->vi_status)
#define VI_ORIGIN (&graphics_plugin->vi_register->vi_origin)
#define VI_WIDTH (&graphics_plugin->vi_register->vi_width)
#define VI_INTR (&graphics_plugin->vi_register->vi_v_intr)
#define VI_V_CURRENT_LINE (&graphics_plugin->vi_register->vi_current)
#define VI_TIMING (&graphics_plugin->vi_register->vi_burst)
#define VI_V_SYNC (&graphics_plugin->vi_register->vi_v_sync)
#define VI_H_SYNC (&graphics_plugin->vi_register->vi_h_sync)
#define VI_LEAP (&graphics_plugin->vi_register->vi_leap)
#define VI_H_START (&graphics_plugin->vi_register->vi_h_start)
#define VI_V_START (&graphics_plugin->vi_register->vi_v_start)
#define VI_V_BURST (&graphics_plugin->vi_register->vi_v_burst)
#define VI_X_SCALE (&graphics_plugin->vi_register->vi_x_scale)
#define VI_Y_SCALE (&graphics_plugin->vi_register->vi_y_scale)

#define DMEM (graphics_plugin->dmem)
#define IMEM (graphics_plugin->imem)
#define RDRAM (graphics_plugin->rdram)

inline u64 TMEM[512];
inline u32 RDRAMSize = 0x800000;
