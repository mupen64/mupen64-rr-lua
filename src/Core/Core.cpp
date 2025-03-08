/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "Core.h"

#include "memory/memory.h"

core_params* g_core{};
std::atomic<int32_t> g_wait_counter = 0;

core_result core_init(core_params* params)
{
    g_core = params;

#define DEFAULT_FUNC(name, func) if (!g_core->callbacks.name) { g_core->callbacks.name = func; g_core->log_warn(std::format(L"Substituting callback {} with default", L#name)); }
    DEFAULT_FUNC(vi, []{});
    DEFAULT_FUNC(input, [](core_buttons*, int){});
    DEFAULT_FUNC(frame, []{});
    DEFAULT_FUNC(interval, []{});
    DEFAULT_FUNC(ai_len_changed, []{});
    DEFAULT_FUNC(play_movie, []{});
    DEFAULT_FUNC(stop_movie, []{});
    DEFAULT_FUNC(save_state, []{});
    DEFAULT_FUNC(load_state, []{});
    DEFAULT_FUNC(reset, []{});
    DEFAULT_FUNC(seek_completed, []{});
    DEFAULT_FUNC(core_executing_changed, [](bool){});
    DEFAULT_FUNC(emu_paused_changed, [](bool){});
    DEFAULT_FUNC(emu_launched_changed, [](bool){});
    DEFAULT_FUNC(emu_starting_changed, [](bool){});
    DEFAULT_FUNC(emu_stopping, []{});
    DEFAULT_FUNC(reset_completed, []{});
    DEFAULT_FUNC(speed_modifier_changed, [](int32_t){});
    DEFAULT_FUNC(warp_modify_status_changed, [](bool){});
    DEFAULT_FUNC(current_sample_changed, [](int32_t){});
    DEFAULT_FUNC(task_changed, [](core_vcr_task){});
    DEFAULT_FUNC(rerecords_changed, [](uint64_t){});
    DEFAULT_FUNC(unfreeze_completed, []{});
    DEFAULT_FUNC(seek_savestate_changed, [](size_t){});
    DEFAULT_FUNC(readonly_changed, [](bool){});
    DEFAULT_FUNC(dacrate_changed, [](core_system_type){});
    DEFAULT_FUNC(debugger_resumed_changed, [](bool){});
    DEFAULT_FUNC(debugger_cpu_state_changed, [](core_dbg_cpu_state*){});
    DEFAULT_FUNC(lag_limit_exceeded, []{});
    DEFAULT_FUNC(seek_status_changed, []{});
    
    g_core->rdram = rdram;
    g_core->rdram_register = &rdram_register;
    g_core->pi_register = &pi_register;
    g_core->MI_register = &MI_register;
    g_core->sp_register = &sp_register;
    g_core->si_register = &si_register;
    g_core->vi_register = &vi_register;
    g_core->rsp_register = &rsp_register;
    g_core->ri_register = &ri_register;
    g_core->ai_register = &ai_register;
    g_core->dpc_register = &dpc_register;
    g_core->dps_register = &dps_register;
    g_core->SP_DMEM = SP_DMEM;
    g_core->SP_IMEM = SP_IMEM;
    g_core->PIF_RAM = PIF_RAM;
    
    return Res_Ok;
}

bool core_vr_get_mge_available()
{
    return g_core->plugin_funcs.video_read_video && g_core->plugin_funcs.video_get_video_size;
}

void core_vr_wait_increment()
{
    ++g_wait_counter;
}

void core_vr_wait_decrement()
{
    --g_wait_counter;
}