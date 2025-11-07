/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.h>
#include <Core.h>
#include <memory/memory.h>
#include <memory/pif.h>
#include <memory/savestates.h>
#include <r4300/debugger.h>
#include <r4300/disasm.h>
#include <r4300/r4300.h>
#include <r4300/rom.h>
#include <r4300/timers.h>
#include <r4300/tracelog.h>
#include <r4300/vcr.h>

core_params *g_core{};
core_ctx g_ctx{};

#if defined(_WIN32)
#define CORE_EXPORT __declspec(dllexport)
#else
#define CORE_EXPORT
#endif

extern "C"
{
    // ReSharper disable CppInconsistentNaming
    CORE_EXPORT void *CORE_RDRAM = nullptr;
    // ReSharper restore CppInconsistentNaming
}

static void log_dummy(const std::string &)
{
}

core_result core_create(core_params *params, core_ctx **ctx)
{
    g_core = params;

    // if (!g_core->io_service)
    // {
    //     return IN_MissingComponent;
    // }

    if (!g_core->log_trace)
    {
        g_core->log_trace = log_dummy;
    }
    if (!g_core->log_info)
    {
        g_core->log_info = log_dummy;
    }
    if (!g_core->log_warn)
    {
        g_core->log_warn = log_dummy;
    }
    if (!g_core->log_error)
    {
        g_core->log_error = log_dummy;
    }

    g_ctx.rdram = rdram;
    g_ctx.rdram_register = &rdram_register;
    g_ctx.pi_register = &pi_register;
    g_ctx.MI_register = &MI_register;
    g_ctx.sp_register = &sp_register;
    g_ctx.si_register = &si_register;
    g_ctx.vi_register = &vi_register;
    g_ctx.rsp_register = &rsp_register;
    g_ctx.ri_register = &ri_register;
    g_ctx.ai_register = &ai_register;
    g_ctx.dpc_register = &dpc_register;
    g_ctx.dps_register = &dps_register;
    g_ctx.SP_DMEM = SP_DMEM;
    g_ctx.SP_IMEM = SP_IMEM;
    g_ctx.PIF_RAM = PIF_RAM;
    CORE_RDRAM = rdram;

    g_ctx.vr_byteswap = rom_byteswap;
    g_ctx.vr_get_rom_path = vr_get_rom_path;
    g_ctx.vr_get_lag_count = [] { return lag_count; };
    g_ctx.vr_get_core_executing = vr_get_core_executing;
    g_ctx.vr_get_launched = vr_get_launched;
    g_ctx.vr_get_frame_advance = vr_get_frame_advance;
    g_ctx.vr_get_paused = vr_get_paused;
    g_ctx.vr_pause_emu = vr_pause_emu;
    g_ctx.vr_resume_emu = vr_resume_emu;
    g_ctx.vr_wait_increment = vr_wait_increment;
    g_ctx.vr_wait_decrement = vr_wait_decrement;
    g_ctx.vr_start_rom = vr_start_rom;
    g_ctx.vr_close_rom = vr_close_rom;
    g_ctx.vr_reset_rom = vr_reset_rom;
    g_ctx.vr_frame_advance = vr_frame_advance;
    g_ctx.vr_set_fast_forward = vr_set_fast_forward;
    g_ctx.vr_get_gs_button = vr_get_gs_button;
    g_ctx.vr_set_gs_button = vr_set_gs_button;
    g_ctx.vr_get_vis_per_second = rom_get_vis_per_second;
    g_ctx.vr_get_rom_header = rom_get_rom_header;
    g_ctx.vr_country_code_to_country_name = rom_country_code_to_country_name;
    g_ctx.vr_on_speed_modifier_changed = timer_on_speed_modifier_changed;
    g_ctx.vr_invalidate_visuals = vr_invalidate_visuals;
    g_ctx.vr_recompile = vr_recompile;
    g_ctx.vr_get_timings = timer_get_timings;
    g_ctx.vcr_parse_header = vcr_parse_header;
    g_ctx.vcr_read_movie_inputs = vcr_read_movie_inputs;
    g_ctx.vcr_start_playback = vcr_start_playback;
    g_ctx.vcr_start_record = vcr_start_record;
    g_ctx.vcr_continue_recording = vcr_continue_recording;
    g_ctx.vcr_replace_author_info = vcr_replace_author_info;
    g_ctx.vcr_get_seek_info = vcr_get_seek_info;
    g_ctx.vcr_begin_seek = vcr_begin_seek;
    g_ctx.vcr_stop_seek = vcr_stop_seek;
    g_ctx.vcr_is_seeking = vcr_is_seeking;
    g_ctx.vcr_write_backup = vcr_write_backup;
    g_ctx.vcr_stop_all = vcr_stop_all;
    g_ctx.vcr_get_path = vcr_get_path;
    g_ctx.vcr_get_task = vcr_get_task;
    g_ctx.vcr_get_length_samples = vcr_get_length_samples;
    g_ctx.vcr_get_length_vis = vcr_get_length_vis;
    g_ctx.vcr_get_current_vi = vcr_get_current_vi;
    g_ctx.vcr_get_inputs = vcr_get_inputs;
    g_ctx.vcr_begin_warp_modify = vcr_begin_warp_modify;
    g_ctx.vcr_get_warp_modify_status = vcr_get_warp_modify_status;
    g_ctx.vcr_get_warp_modify_first_difference_frame = vcr_get_warp_modify_first_difference_frame;
    g_ctx.vcr_get_seek_savestate_frames = vcr_get_seek_savestate_frames;
    g_ctx.vcr_has_seek_savestate_at_frame = vcr_has_seek_savestate_at_frame;
    g_ctx.tl_active = tl_active;
    g_ctx.tl_start = tl_start;
    g_ctx.tl_stop = tl_stop;
    g_ctx.st_do_file = st_do_file;
    g_ctx.st_do_memory = st_do_memory;
    g_ctx.st_get_undo_savestate = st_get_undo_savestate;
    g_ctx.dbg_get_resumed = dbg_get_resumed;
    g_ctx.dbg_set_is_resumed = dbg_set_is_resumed;
    g_ctx.dbg_step = dbg_step;
    g_ctx.dbg_get_dma_read_enabled = dbg_get_dma_read_enabled;
    g_ctx.dbg_set_dma_read_enabled = dbg_set_dma_read_enabled;
    g_ctx.dbg_get_rsp_enabled = dbg_get_rsp_enabled;
    g_ctx.dbg_set_rsp_enabled = dbg_set_rsp_enabled;
    g_ctx.dbg_disassemble = dbg_disassemble;

    *ctx = &g_ctx;

    return Res_Ok;
}
