/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Plugin.hpp"
#include <cstring>
#include <future>
#include <stdexcept>
#include <type_traits>

#include <boost/dll.hpp>
#include <boost/filesystem/detail/path_traits.hpp>

#include "core_api.h"
#include "core_plugin.h"
#include "mupapi.h"

#include "Core.hpp"
#include "Logging.hpp"

namespace dll = boost::dll;
namespace bfs = boost::filesystem;

#define MUP_FN(name) get<std::remove_pointer_t<fp_##name>>(#name)
#define MUP_GET(dest, lib, name, dummy)                                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        if (lib.has(#name))                                                                                            \
        {                                                                                                              \
            dest = lib.get<std::remove_pointer_t<fp_##name>>(#name);                                                   \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            dest = dummy;                                                                                              \
        }                                                                                                              \
    } while (false);
#define MUP_GET2(lib, name, dummy) (lib.has(#name)) ? lib.get<std::remove_pointer_t<fp_##name>>(#name) : dummy

// DUMMY FUNCTIONS
// =============================================

static uint32_t CALL dummy_do_rsp_cycles(uint32_t Cycles)
{
    return Cycles;
}

static void CALL dummy_void()
{
}

static int32_t CALL dummy_mupv_init(core_gfx_info)
{
    return 1;
}

static int32_t CALL dummy_mupa_init(core_audio_info)
{
    return 1;
}

static void CALL dummy_mupi_init(core_input_info)
{
}

static void CALL dummy_mupr_init(core_rsp_info, uint32_t *)
{
}

static void CALL dummy_ai_dacrate_changed(int32_t)
{
}

static uint32_t CALL dummy_ai_read_length()
{
    return 0;
}

static void CALL dummy_ai_update(int32_t)
{
}

static void CALL dummy_controller_command(int32_t, uint8_t *)
{
}

static void CALL dummy_get_keys(int32_t, core_buttons *)
{
}

static void CALL dummy_set_keys(int32_t, core_buttons)
{
}

static void CALL dummy_read_controller(int32_t, uint8_t *)
{
}

static void CALL dummy_key_down(uint32_t, int32_t)
{
}

static void CALL dummy_key_up(uint32_t, int32_t)
{
}

static void CALL dummy_fb_read(uint32_t)
{
}

static void CALL dummy_fb_write(uint32_t, uint32_t)
{
}

static void CALL dummy_get_fb_info(void *)
{
}

static void CALL dummy_move_screen(int32_t, int32_t)
{
}

// logging callbacks for plugins
// =============================================
static core_plugin_extended_funcs video_ext_funcs{
    .size = sizeof(core_plugin_extended_funcs),
    .log_trace = [](const char *msg) { Mupen::video_log().trace(msg); },
    .log_info = [](const char *msg) { Mupen::video_log().info(msg); },
    .log_warn = [](const char *msg) { Mupen::video_log().warn(msg); },
    .log_error = [](const char *msg) { Mupen::video_log().error(msg); },
};
static core_plugin_extended_funcs audio_ext_funcs{
    .size = sizeof(core_plugin_extended_funcs),
    .log_trace = [](const char *msg) { Mupen::audio_log().trace(msg); },
    .log_info = [](const char *msg) { Mupen::audio_log().info(msg); },
    .log_warn = [](const char *msg) { Mupen::audio_log().warn(msg); },
    .log_error = [](const char *msg) { Mupen::audio_log().error(msg); },
};
static core_plugin_extended_funcs input_ext_funcs{
    .size = sizeof(core_plugin_extended_funcs),
    .log_trace = [](const char *msg) { Mupen::input_log().trace(msg); },
    .log_info = [](const char *msg) { Mupen::input_log().info(msg); },
    .log_warn = [](const char *msg) { Mupen::input_log().warn(msg); },
    .log_error = [](const char *msg) { Mupen::input_log().error(msg); },
};
static core_plugin_extended_funcs rsp_ext_funcs{
    .size = sizeof(core_plugin_extended_funcs),
    .log_trace = [](const char *msg) { Mupen::rsp_log().trace(msg); },
    .log_info = [](const char *msg) { Mupen::rsp_log().info(msg); },
    .log_warn = [](const char *msg) { Mupen::rsp_log().warn(msg); },
    .log_error = [](const char *msg) { Mupen::rsp_log().error(msg); },
};

namespace Mupen
{
PluginInfo extract_plugin_info(const std::filesystem::path &path)
{

    core_plugin_info info{};

    // Load the plugin just to call the get_info function.
    {
        auto plugin_lib = dll::shared_library(bfs::path(path));
        auto mup_get_info = plugin_lib.get<fp_mup_get_info>("mup_get_info");
        mup_get_info(&info);
    }

    return {.path = path, .info = std::move(info)};
}

PluginSet::PluginSet(std::filesystem::path video_path, std::filesystem::path audio_path,
                     std::filesystem::path input_path, std::filesystem::path rsp_path)
    : m_video_plugin(bfs::path(video_path)), m_audio_plugin(bfs::path(audio_path)),
      m_input_plugin(bfs::path(input_path)), m_rsp_plugin(bfs::path(rsp_path))
{
    // check that all 4 plugins are the correct type
    core_plugin_info info = {};

    m_video_get_info = m_video_plugin.MUP_FN(mup_get_info);
    m_audio_get_info = m_audio_plugin.MUP_FN(mup_get_info);
    m_input_get_info = m_input_plugin.MUP_FN(mup_get_info);
    m_rsp_get_info = m_rsp_plugin.MUP_FN(mup_get_info);

    m_video_get_info(&info);
    core_log().info("video plugin type: {}", (int)info.type);
    if (info.type != plugin_video) throw std::invalid_argument("video plugin path does not point to a video plugin");

    info = {};
    m_audio_get_info(&info);
    core_log().info("audio plugin type: {}", (int)info.type);
    if (info.type != plugin_audio) throw std::invalid_argument("audio plugin path does not point to an audio plugin");

    info = {};
    m_input_get_info(&info);
    if (info.type != plugin_input) throw std::invalid_argument("input plugin path does not point to an input plugin");

    info = {};
    m_rsp_get_info(&info);
    if (info.type != plugin_rsp) throw std::invalid_argument("RSP plugin path does not point to an RSP plugin");

    auto exe_path_str = boost::dll::program_location().string();

    m_video_plugin.MUP_FN(mup_init)(exe_path_str.c_str(), &video_ext_funcs);
    m_audio_plugin.MUP_FN(mup_init)(exe_path_str.c_str(), &audio_ext_funcs);
    m_input_plugin.MUP_FN(mup_init)(exe_path_str.c_str(), &input_ext_funcs);
    m_rsp_plugin.MUP_FN(mup_init)(exe_path_str.c_str(), &rsp_ext_funcs);

    // resolve common functions that *we* care about
    m_rom_opened_all = {
        MUP_GET2(m_video_plugin, mup_rom_opened, dummy_void),
        MUP_GET2(m_audio_plugin, mup_rom_opened, dummy_void),
        MUP_GET2(m_input_plugin, mup_rom_opened, dummy_void),
        MUP_GET2(m_rsp_plugin, mup_rom_opened, dummy_void),
    };
    m_rom_closed_all = {
        MUP_GET2(m_video_plugin, mup_rom_closed, dummy_void),
        MUP_GET2(m_audio_plugin, mup_rom_closed, dummy_void),
        MUP_GET2(m_input_plugin, mup_rom_closed, dummy_void),
        MUP_GET2(m_rsp_plugin, mup_rom_closed, dummy_void),
    };
}

PluginSet::~PluginSet()
{
    // This is called to free any remaining resources before unloading the libraries.
    if (m_video_plugin.is_loaded()) m_video_plugin.MUP_FN(mup_drop)();
    if (m_audio_plugin.is_loaded()) m_audio_plugin.MUP_FN(mup_drop)();
    if (m_input_plugin.is_loaded()) m_input_plugin.MUP_FN(mup_drop)();
    if (m_rsp_plugin.is_loaded()) m_rsp_plugin.MUP_FN(mup_drop)();
}

void PluginSet::extract_names(char *video, char *audio, char *input, char *rsp)
{
    core_plugin_info info;

    if (video != nullptr)
    {
        info = {};
        m_video_get_info(&info);
        strncpy(video, info.name, 64);
    }

    if (audio != nullptr)
    {
        info = {};
        m_audio_get_info(&info);
        strncpy(audio, info.name, 64);
    }

    if (input != nullptr)
    {
        info = {};
        m_input_get_info(&info);
        strncpy(input, info.name, 64);
    }

    if (rsp != nullptr)
    {
        info = {};
        m_rsp_get_info(&info);
        strncpy(rsp, info.name, 64);
    }
}

void PluginSet::resolve_functions_to(core_params &params)
{
    // video plugin
    MUP_GET(params.video_process_dlist, m_video_plugin, mupv_process_d_list, dummy_void);
    MUP_GET(params.video_process_rdp_list, m_video_plugin, mupv_process_rdp_list, dummy_void);
    MUP_GET(params.video_show_cfb, m_video_plugin, mupv_show_cfb, dummy_void);
    MUP_GET(params.video_vi_status_changed, m_video_plugin, mupv_vi_status_changed, dummy_void);
    MUP_GET(params.video_vi_width_changed, m_video_plugin, mupv_vi_width_changed, dummy_void);
    MUP_GET(params.video_fb_read, m_video_plugin, mupv_fb_read, dummy_fb_read);
    MUP_GET(params.video_fb_write, m_video_plugin, mupv_fb_write, dummy_fb_write);
    MUP_GET(params.video_fb_get_frame_buffer_info, m_video_plugin, mupv_get_fb_info, dummy_get_fb_info);

    // audio plugin
    MUP_GET(params.audio_ai_dacrate_changed, m_audio_plugin, mupa_ai_dacrate_changed, dummy_ai_dacrate_changed);
    MUP_GET(params.audio_ai_len_changed, m_audio_plugin, mupa_ai_len_changed, dummy_void);
    MUP_GET(params.audio_ai_read_length, m_audio_plugin, mupa_ai_read_length, dummy_ai_read_length);
    MUP_GET(params.audio_process_alist, m_audio_plugin, mupa_process_a_list, dummy_void);
    MUP_GET(params.audio_ai_update, m_audio_plugin, mupa_ai_update, dummy_ai_update);

    // input plugin
    MUP_GET(params.input_controller_command, m_input_plugin, mupi_controller_command, dummy_controller_command);
    MUP_GET(params.input_get_keys, m_input_plugin, mupi_get_keys, dummy_get_keys);
    MUP_GET(params.input_set_keys, m_input_plugin, mupi_set_keys, dummy_set_keys);
    MUP_GET(params.input_read_controller, m_input_plugin, mupi_read_controller, dummy_read_controller);

    // rsp plugin
    MUP_GET(params.rsp_do_rsp_cycles, m_rsp_plugin, mupr_do_rsp_cycles, dummy_do_rsp_cycles);
}

void PluginSet::initiate_video(core_ctx &ctx, ICoreService &core_service)
{
    if (!m_video_plugin.has("mupv_init")) return;
    auto gfx_info = core_gfx_info{
        .byteswapped = 1,
        .rom = ctx.rom,
        .rdram = (uint8_t *)ctx.rdram,
        .dmem = (uint8_t *)ctx.SP_DMEM,
        .imem = (uint8_t *)ctx.SP_IMEM,
        .mi_intr_reg = &ctx.MI_register->mi_intr_reg,
        .dpc_start_reg = &ctx.dpc_register->dpc_start,
        .dpc_end_reg = &ctx.dpc_register->dpc_end,
        .dpc_current_reg = &ctx.dpc_register->dpc_current,
        .dpc_status_reg = &ctx.dpc_register->dpc_status,
        .dpc_clock_reg = &ctx.dpc_register->dpc_clock,
        .dpc_bufbusy_reg = &ctx.dpc_register->dpc_bufbusy,
        .dpc_pipebusy_reg = &ctx.dpc_register->dpc_pipebusy,
        .dpc_tmem_reg = &ctx.dpc_register->dpc_tmem,
        .vi_status_reg = &ctx.vi_register->vi_status,
        .vi_origin_reg = &ctx.vi_register->vi_origin,
        .vi_width_reg = &ctx.vi_register->vi_width,
        .vi_intr_reg = &ctx.vi_register->vi_v_intr,
        .vi_v_current_line_reg = &ctx.vi_register->vi_current,
        .vi_timing_reg = &ctx.vi_register->vi_burst,
        .vi_v_sync_reg = &ctx.vi_register->vi_v_sync,
        .vi_h_sync_reg = &ctx.vi_register->vi_h_sync,
        .vi_leap_reg = &ctx.vi_register->vi_leap,
        .vi_h_start_reg = &ctx.vi_register->vi_h_start,
        .vi_v_start_reg = &ctx.vi_register->vi_v_start,
        .vi_v_burst_reg = &ctx.vi_register->vi_v_burst,
        .vi_x_scale_reg = &ctx.vi_register->vi_x_scale,
        .vi_y_scale_reg = &ctx.vi_register->vi_y_scale,
    };
    // init and request window settings
    auto wm_settings = mupv_wm_settings_default();
    m_video_plugin.MUP_FN(mupv_init)(gfx_info, &wm_settings);

    // pass child window to gfx
    if (wm_settings.backend == MUPV_BK_NONE) return;
    auto wm_handle = core_service.setup_window(wm_settings);
    m_video_plugin.MUP_FN(mupv_receive_child_window)(wm_handle);
}

void PluginSet::initiate_audio(core_ctx &ctx)
{
    if (!m_audio_plugin.has("mupa_init")) return;
    auto audio_info = core_audio_info{
        .byteswapped = 1,
        .rom = ctx.rom,
        .dmem = (uint8_t *)ctx.SP_DMEM,
        .imem = (uint8_t *)ctx.SP_IMEM,
        .mi_intr_reg = &ctx.MI_register->mi_intr_reg,
        .ai_dram_addr_reg = &ctx.ai_register->ai_dram_addr,
        .ai_len_reg = &ctx.ai_register->ai_len,
        .ai_control_reg = &ctx.ai_register->ai_control,
        .ai_status_reg = &ctx.ai_register->ai_status,
        .ai_dacrate_reg = &ctx.ai_register->ai_dacrate,
        .ai_bitrate_reg = &ctx.ai_register->ai_bitrate,
        .check_interrupts = dummy_void,
    };
    m_audio_plugin.MUP_FN(mupa_init)(audio_info);
}

void PluginSet::initiate_input(core_ctx &ctx, core_params &params)
{
    auto input_info = core_input_info{
        .byteswapped = 1,
        .header = ctx.rom,
        .controllers = params.controls,
    };
    if (!m_input_plugin.has("mupi_init")) return;
    m_input_plugin.MUP_FN(mupi_init)(input_info);
}

void PluginSet::initiate_rsp(core_ctx &ctx, core_params &params)
{
    if (!m_rsp_plugin.has("mupr_init")) return;
    auto rsp_info = core_rsp_info{
        .byteswapped = 1,
        .rdram = (uint8_t *)ctx.rdram,
        .dmem = (uint8_t *)ctx.SP_DMEM,
        .imem = (uint8_t *)ctx.SP_IMEM,
        .mi_intr_reg = &ctx.MI_register->mi_intr_reg,
        .sp_mem_addr_reg = &ctx.sp_register->sp_mem_addr_reg,
        .sp_dram_addr_reg = &ctx.sp_register->sp_dram_addr_reg,
        .sp_rd_len_reg = &ctx.sp_register->sp_rd_len_reg,
        .sp_wr_len_reg = &ctx.sp_register->sp_wr_len_reg,
        .sp_status_reg = &ctx.sp_register->sp_status_reg,
        .sp_dma_full_reg = &ctx.sp_register->sp_dma_full_reg,
        .sp_dma_busy_reg = &ctx.sp_register->sp_dma_busy_reg,
        .sp_pc_reg = &ctx.rsp_register->rsp_pc,
        .sp_semaphore_reg = &ctx.sp_register->sp_semaphore_reg,
        .dpc_start_reg = &ctx.dpc_register->dpc_start,
        .dpc_end_reg = &ctx.dpc_register->dpc_end,
        .dpc_current_reg = &ctx.dpc_register->dpc_current,
        .dpc_status_reg = &ctx.dpc_register->dpc_status,
        .dpc_clock_reg = &ctx.dpc_register->dpc_clock,
        .dpc_bufbusy_reg = &ctx.dpc_register->dpc_bufbusy,
        .dpc_pipebusy_reg = &ctx.dpc_register->dpc_pipebusy,
        .dpc_tmem_reg = &ctx.dpc_register->dpc_tmem,
        .check_interrupts = dummy_void,
        .process_dlist_list = params.video_process_dlist,
        .process_alist_list = params.audio_process_alist,
        .process_rdp_list = params.video_process_rdp_list,
        .show_cfb = params.video_show_cfb,
    };
    m_rsp_plugin.MUP_FN(mupr_init)(rsp_info);
}

void PluginSet::initiate_all(core_ctx &ctx, core_params &params, ICoreService &core_service)
{
    // run all inits concurrently. wait until they finish.
    auto video_init = std::async([&]() { initiate_video(ctx, core_service); });
    auto audio_init = std::async([&]() { initiate_audio(ctx); });
    auto input_init = std::async([&]() { initiate_input(ctx, params); });
    auto rsp_init = std::async([&]() { initiate_rsp(ctx, params); });

    video_init.wait();
    audio_init.wait();
    input_init.wait();
    rsp_init.wait();
}

void PluginSet::call_rom_opened()
{
    for (auto mup_rom_opened : m_rom_opened_all)
    {
        mup_rom_opened();
    }
}

void PluginSet::call_rom_closed()
{
    for (auto mup_rom_closed : m_rom_closed_all)
    {
        mup_rom_closed();
    }
}

} // namespace Mupen