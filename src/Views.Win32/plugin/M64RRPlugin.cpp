/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Config.hpp>
#include <DialogService.hpp>
#include <components/Statusbar.hpp>
#include <plugin/M64RRPlugin.hpp>
#include <plugin/Plugin.hpp>

static CoreController controller_to_core_controller(const M64RRSpec::Controller &controller)
{
    CoreControllerExtension extension;
    switch (controller.plugin)
    {
    case M64RRSpec::ControllerExtension::None:
        extension = CoreControllerExtension::None;
        break;
    case M64RRSpec::ControllerExtension::Mempak:
        extension = CoreControllerExtension::Mempak;
        break;
    case M64RRSpec::ControllerExtension::Rumblepak:
        extension = CoreControllerExtension::Rumblepak;
        break;
    case M64RRSpec::ControllerExtension::Transferpak:
        extension = CoreControllerExtension::Transferpak;
        break;
    case M64RRSpec::ControllerExtension::Raw:
        extension = CoreControllerExtension::Raw;
        break;
    default:
        RT_ASSERT(false, L"Unknown controller extension");
        break;
    }

    return CoreController{
        .Present = controller.present ? 1 : 0,
        .RawData = controller.raw ? 1 : 0,
        .Plugin = extension,
    };
}

static M64RRSpec::PtrRomOpened s_mupenrr_rom_opened_fn = nullptr;
static M64RRSpec::PtrRomClosed s_mupenrr_rom_closed_fn = nullptr;
static M64RRSpec::PtrProcessDList s_mupenrr_process_dlist_fn = nullptr;
static M64RRSpec::PtrGetVideoSize s_mupenrr_get_video_size_fn = nullptr;
static M64RRSpec::PtrReadVideo s_mupenrr_read_video_fn = nullptr;
static M64RRSpec::PtrAIDacrateChanged s_mupenrr_ai_dacrate_changed_fn = nullptr;
static M64RRSpec::PtrAILenChanged s_mupenrr_ai_len_changed_fn = nullptr;
static M64RRSpec::PtrGetKeys s_mupenrr_get_keys_fn = nullptr;
static M64RRSpec::PtrSetKeys s_mupenrr_set_keys_fn = nullptr;
static M64RRSpec::PtrReadController s_mupenrr_read_controller_fn = nullptr;
static M64RRSpec::PtrDoRSPCycles s_mupenrr_do_rsp_cycles_fn = nullptr;
static M64RRSpec::PtrShutdown s_mupenrr_shutdown_fn = nullptr;

#define LOOKUP_MUPENRR_FN(mupenrr_ptr, mupenrr_type, export_name)                                                      \
    mupenrr_ptr = (mupenrr_type)GetProcAddress(m_module, export_name);

std::pair<std::wstring, std::unique_ptr<Plugin>> M64RRPlugin::create(HMODULE module, std::filesystem::path path)
{
    const auto get_metadata = (M64RRSpec::PtrGetMetadata)GetProcAddress(module, "M64RRGetMetadata");

    if (!get_metadata)
    {
        return std::make_pair(L"M64RRGetMetadata missing", nullptr);
    }

    M64RRSpec::PluginMetadata metadata{};
    get_metadata(&metadata);

    const size_t target_version_len = strnlen(metadata.target_version, std::size(metadata.target_version));
    if (target_version_len > 0)
    {
        // Plugin is tied to one version of mupen
        const auto current_version = IOUtils::to_utf8_string(CURRENT_VERSION);
        const std::string target_version(metadata.target_version, target_version_len);
        if (current_version != target_version)
        {
            return std::make_pair(L"Incompatible with this version of Mupen64", nullptr);
        }
    }

    const size_t plugin_name_len = strlen(metadata.name);
    while (plugin_name_len > 0 && metadata.name[plugin_name_len - 1] == ' ')
    {
        metadata.name[plugin_name_len - 1] = '\0';
    }

    auto plugin = std::make_unique<M64RRPlugin>();

    plugin->m_path = path;
    plugin->m_name = std::format("{} (✅)", metadata.name);
    switch (metadata.type)
    {
    case M64RRSpec::PluginType::Video:
        plugin->m_type = Plugin::Type::Video;
        break;
    case M64RRSpec::PluginType::Audio:
        plugin->m_type = Plugin::Type::Audio;
        break;
    case M64RRSpec::PluginType::Input:
        plugin->m_type = Plugin::Type::Input;
        break;
    case M64RRSpec::PluginType::RSP:
        plugin->m_type = Plugin::Type::RSP;
        break;
    }
    plugin->m_module = module;
    plugin->m_meta = metadata;

    g_view_logger->info("[Plugin] Created plugin {}", plugin->m_name);
    return std::make_pair(L"", std::move(plugin));
}

void M64RRPlugin::config(HWND hwnd)
{
    initiate_dummy();

    const auto show_config = (M64RRSpec::PtrShowConfig)GetProcAddress(m_module, "M64RRRShowConfig");

    if (show_config)
        show_config(hwnd);
    else
    {
        DialogService::show_dialog(
            std::format(L"'{}' has no configuration.", IOUtils::to_wide_string(this->name())).c_str(), L"Plugin",
            fsvc_error, hwnd);
    }

    deinitiate_dummy();
}

void M64RRPlugin::test(HWND hwnd)
{
}

void M64RRPlugin::about(HWND hwnd)
{
    MessageBox(hwnd, IOUtils::to_wide_string(m_meta.description).c_str(), L"About", MB_ICONINFORMATION | MB_OK);
}

void M64RRPlugin::initiate()
{
    auto initiate_fn = (M64RRSpec::PtrInitiate)GetProcAddress(m_module, "M64RRInitiate");
    if (!initiate_fn) initiate_fn = [](M64RRSpec::PluginInit *init) {};

    M64RRSpec::PluginInit init{};

    init.platform = M64RRSpec::Platform::Windows;
    init.byteswapped = 1;
    init.rom = g_main_ctx.core_ctx->rom;
    init.rdram = (uint8_t *)g_main_ctx.core_ctx->rdram;
    init.dmem = (uint8_t *)g_main_ctx.core_ctx->SP_DMEM;
    init.imem = (uint8_t *)g_main_ctx.core_ctx->SP_IMEM;
    init.mi_intr_reg = &g_main_ctx.core_ctx->MI_register->mi_intr_reg;
    init.dpc_start_reg = &g_main_ctx.core_ctx->dpc_register->dpc_start;
    init.dpc_end_reg = &g_main_ctx.core_ctx->dpc_register->dpc_end;
    init.dpc_current_reg = &g_main_ctx.core_ctx->dpc_register->dpc_current;
    init.dpc_status_reg = &g_main_ctx.core_ctx->dpc_register->dpc_status;
    init.dpc_clock_reg = &g_main_ctx.core_ctx->dpc_register->dpc_clock;
    init.dpc_bufbusy_reg = &g_main_ctx.core_ctx->dpc_register->dpc_bufbusy;
    init.dpc_pipebusy_reg = &g_main_ctx.core_ctx->dpc_register->dpc_pipebusy;
    init.dpc_tmem_reg = &g_main_ctx.core_ctx->dpc_register->dpc_tmem;
    init.vi_status_reg = &g_main_ctx.core_ctx->vi_register->vi_status;
    init.vi_origin_reg = &g_main_ctx.core_ctx->vi_register->vi_origin;
    init.vi_width_reg = &g_main_ctx.core_ctx->vi_register->vi_width;
    init.vi_intr_reg = &g_main_ctx.core_ctx->vi_register->vi_v_intr;
    init.vi_v_current_line_reg = &g_main_ctx.core_ctx->vi_register->vi_current;
    init.vi_timing_reg = &g_main_ctx.core_ctx->vi_register->vi_burst;
    init.vi_v_sync_reg = &g_main_ctx.core_ctx->vi_register->vi_v_sync;
    init.vi_h_sync_reg = &g_main_ctx.core_ctx->vi_register->vi_h_sync;
    init.vi_leap_reg = &g_main_ctx.core_ctx->vi_register->vi_leap;
    init.vi_h_start_reg = &g_main_ctx.core_ctx->vi_register->vi_h_start;
    init.vi_v_start_reg = &g_main_ctx.core_ctx->vi_register->vi_v_start;
    init.vi_v_burst_reg = &g_main_ctx.core_ctx->vi_register->vi_v_burst;
    init.vi_x_scale_reg = &g_main_ctx.core_ctx->vi_register->vi_x_scale;
    init.vi_y_scale_reg = &g_main_ctx.core_ctx->vi_register->vi_y_scale;
    init.ai_dram_addr_reg = &g_main_ctx.core_ctx->ai_register->ai_dram_addr;
    init.ai_len_reg = &g_main_ctx.core_ctx->ai_register->ai_len;
    init.ai_control_reg = &g_main_ctx.core_ctx->ai_register->ai_control;
    init.ai_status_reg = &dummy_dw;
    init.ai_dacrate_reg = &g_main_ctx.core_ctx->ai_register->ai_dacrate;
    init.ai_bitrate_reg = &g_main_ctx.core_ctx->ai_register->ai_bitrate;
    init.sp_mem_addr_reg = &g_main_ctx.core_ctx->sp_register->sp_mem_addr_reg;
    init.sp_dram_addr_reg = &g_main_ctx.core_ctx->sp_register->sp_dram_addr_reg;
    init.sp_rd_len_reg = &g_main_ctx.core_ctx->sp_register->sp_rd_len_reg;
    init.sp_wr_len_reg = &g_main_ctx.core_ctx->sp_register->sp_wr_len_reg;
    init.sp_status_reg = &g_main_ctx.core_ctx->sp_register->sp_status_reg;
    init.sp_dma_full_reg = &g_main_ctx.core_ctx->sp_register->sp_dma_full_reg;
    init.sp_dma_busy_reg = &g_main_ctx.core_ctx->sp_register->sp_dma_busy_reg;
    init.sp_pc_reg = &g_main_ctx.core_ctx->rsp_register->rsp_pc;
    init.sp_semaphore_reg = &g_main_ctx.core_ctx->sp_register->sp_semaphore_reg;
    init.process_dlist = g_plugin_funcs.video_process_dlist;
    init.header = g_main_ctx.core_ctx->rom;

    std::array<M64RRSpec::Controller, 4> tmp_controllers{};
    init.controllers = tmp_controllers.data();

    switch (m_type)
    {
    case Plugin::Type::Video:
        g_plugin_funcs.video_extended_funcs = GEN_EXTENDED_FUNCS(g_video_logger);
        init.ef = &g_plugin_funcs.video_extended_funcs;
        break;
    case Plugin::Type::Audio:
        g_plugin_funcs.audio_extended_funcs = GEN_EXTENDED_FUNCS(g_audio_logger);
        init.ef = &g_plugin_funcs.audio_extended_funcs;
        break;
    case Plugin::Type::Input:
        g_plugin_funcs.input_extended_funcs = GEN_EXTENDED_FUNCS(g_input_logger);
        init.ef = &g_plugin_funcs.input_extended_funcs;
        break;
    case Plugin::Type::RSP:
        g_plugin_funcs.rsp_extended_funcs = GEN_EXTENDED_FUNCS(g_rsp_logger);
        init.ef = &g_plugin_funcs.rsp_extended_funcs;
        break;
    }

    initiate_fn(&init);

    switch (m_type)
    {
    case Plugin::Type::Video: {
        g_view_logger->trace("Initiating video plugin (MupenRR)...");

        LOOKUP_MUPENRR_FN(s_mupenrr_rom_opened_fn, M64RRSpec::PtrRomOpened, "M64RRRomOpened");
        LOOKUP_MUPENRR_FN(s_mupenrr_rom_closed_fn, M64RRSpec::PtrRomClosed, "M64RRRomClosed");
        LOOKUP_MUPENRR_FN(s_mupenrr_process_dlist_fn, M64RRSpec::PtrProcessDList, "M64RRProcessDList");
        LOOKUP_MUPENRR_FN(s_mupenrr_get_video_size_fn, M64RRSpec::PtrGetVideoSize, "M64RRGetVideoSize");
        LOOKUP_MUPENRR_FN(s_mupenrr_read_video_fn, M64RRSpec::PtrReadVideo, "M64RRReadVideo");
        LOOKUP_MUPENRR_FN(s_mupenrr_shutdown_fn, M64RRSpec::PtrShutdown, "M64RRShutdown");

        g_plugin_funcs.video_rom_open = []() {
            if (s_mupenrr_rom_opened_fn) s_mupenrr_rom_opened_fn();
        };
        g_plugin_funcs.video_rom_closed = []() {
            if (s_mupenrr_rom_closed_fn) s_mupenrr_rom_closed_fn();
        };
        g_plugin_funcs.video_close_dll = []() {
            if (s_mupenrr_shutdown_fn) s_mupenrr_shutdown_fn();
        };
        g_plugin_funcs.video_process_dlist = []() {
            if (s_mupenrr_process_dlist_fn) s_mupenrr_process_dlist_fn();
        };
        g_plugin_funcs.video_process_rdp_list = [](const auto &...) {};
        g_plugin_funcs.video_show_cfb = [](const auto &...) {};
        g_plugin_funcs.video_vi_status_changed = [](const auto &...) {};
        g_plugin_funcs.video_vi_width_changed = [](const auto &...) {};
        if (s_mupenrr_get_video_size_fn) g_plugin_funcs.video_get_video_size = s_mupenrr_get_video_size_fn;
        if (s_mupenrr_read_video_fn) g_plugin_funcs.video_read_video = s_mupenrr_read_video_fn;
        g_plugin_funcs.video_change_window = [](const auto &...) {};
        g_plugin_funcs.video_update_screen = [](const auto &...) {};
        g_plugin_funcs.video_move_screen = [](int32_t, int32_t) {};
        g_plugin_funcs.video_capture_screen = [](char *) {};
        g_plugin_funcs.video_read_screen = nullptr;
        g_plugin_funcs.video_fb_read = [](uint32_t) {};
        g_plugin_funcs.video_fb_write = [](uint32_t, uint32_t) {};
        g_plugin_funcs.video_fb_get_frame_buffer_info = [](ZESpec::FBInfo *) {};
        g_plugin_funcs.video_dll_crt_free = PluginUtil::get_free_function_in_module(m_module);

        break;
    }
    case Plugin::Type::Audio: {
        g_view_logger->trace("Initiating audio plugin (MupenRR)...");

        LOOKUP_MUPENRR_FN(s_mupenrr_rom_opened_fn, M64RRSpec::PtrRomOpened, "M64RRRomOpened");
        LOOKUP_MUPENRR_FN(s_mupenrr_rom_closed_fn, M64RRSpec::PtrRomClosed, "M64RRRomClosed");
        LOOKUP_MUPENRR_FN(s_mupenrr_ai_dacrate_changed_fn, M64RRSpec::PtrAIDacrateChanged,
                          "M64RRAIDacrateChanged");
        LOOKUP_MUPENRR_FN(s_mupenrr_ai_len_changed_fn, M64RRSpec::PtrAILenChanged, "M64RRAILenChanged");
        LOOKUP_MUPENRR_FN(s_mupenrr_shutdown_fn, M64RRSpec::PtrShutdown, "M64RRShutdown");

        g_plugin_funcs.audio_rom_open = []() {
            if (s_mupenrr_rom_opened_fn) s_mupenrr_rom_opened_fn();
        };
        g_plugin_funcs.audio_rom_closed = []() {
            if (s_mupenrr_rom_closed_fn) s_mupenrr_rom_closed_fn();
        };
        g_plugin_funcs.audio_close_dll_audio = []() {
            if (s_mupenrr_shutdown_fn) s_mupenrr_shutdown_fn();
        };
        g_plugin_funcs.audio_ai_dacrate_changed = [](int32_t system_type) {
            if (s_mupenrr_ai_dacrate_changed_fn) s_mupenrr_ai_dacrate_changed_fn(system_type);
        };
        g_plugin_funcs.audio_ai_len_changed = []() {
            if (s_mupenrr_ai_len_changed_fn) s_mupenrr_ai_len_changed_fn();
        };
        g_plugin_funcs.audio_ai_read_length = []() { return 0u; };
        g_plugin_funcs.audio_process_alist = [](const auto &...) {};
        g_plugin_funcs.audio_ai_update = [](int32_t) {};

        break;
    }
    case Plugin::Type::Input: {
        g_view_logger->trace("Initiating input plugin (MupenRR)...");

        LOOKUP_MUPENRR_FN(s_mupenrr_rom_opened_fn, M64RRSpec::PtrRomOpened, "M64RRRomOpened");
        LOOKUP_MUPENRR_FN(s_mupenrr_rom_closed_fn, M64RRSpec::PtrRomClosed, "M64RRRomClosed");
        LOOKUP_MUPENRR_FN(s_mupenrr_get_keys_fn, M64RRSpec::PtrGetKeys, "M64RRGetKeys");
        LOOKUP_MUPENRR_FN(s_mupenrr_set_keys_fn, M64RRSpec::PtrSetKeys, "M64RRSetKeys");
        LOOKUP_MUPENRR_FN(s_mupenrr_read_controller_fn, M64RRSpec::PtrReadController, "M64RRReadController");
        LOOKUP_MUPENRR_FN(s_mupenrr_shutdown_fn, M64RRSpec::PtrShutdown, "M64RRShutdown");

        g_plugin_funcs.input_rom_open = []() {
            if (s_mupenrr_rom_opened_fn) s_mupenrr_rom_opened_fn();
        };
        g_plugin_funcs.input_rom_closed = []() {
            if (s_mupenrr_rom_closed_fn) s_mupenrr_rom_closed_fn();
        };
        g_plugin_funcs.input_close_dll = []() {
            if (s_mupenrr_shutdown_fn) s_mupenrr_shutdown_fn();
        };
        g_plugin_funcs.input_controller_command = [](int32_t, uint8_t *) {};
        g_plugin_funcs.input_get_keys = [](int32_t controller, ZESpec::Buttons *keys) {
            if (s_mupenrr_get_keys_fn) s_mupenrr_get_keys_fn(controller, (M64RRSpec::Buttons *)keys);
        };
        g_plugin_funcs.input_set_keys = [](int32_t controller, ZESpec::Buttons keys) {
            if (s_mupenrr_set_keys_fn) s_mupenrr_set_keys_fn(controller, *(M64RRSpec::Buttons *)&keys);
        };
        g_plugin_funcs.input_read_controller = [](int32_t controller, unsigned char *command) {
            if (s_mupenrr_read_controller_fn) s_mupenrr_read_controller_fn(controller, command);
        };
        g_plugin_funcs.input_key_down = [](uint32_t, int32_t) {};
        g_plugin_funcs.input_key_up = [](uint32_t, int32_t) {};

        for (size_t i = 0; i < std::size(tmp_controllers); ++i)
        {
            g_main_ctx.core.controls[i] = controller_to_core_controller(tmp_controllers[i]);
        }
        break;
    }
    case Plugin::Type::RSP: {
        g_view_logger->trace("Initiating RSP plugin (MupenRR)...");

        LOOKUP_MUPENRR_FN(s_mupenrr_rom_closed_fn, M64RRSpec::PtrRomClosed, "M64RRRomClosed");
        LOOKUP_MUPENRR_FN(s_mupenrr_do_rsp_cycles_fn, M64RRSpec::PtrDoRSPCycles, "M64RRDoRSPCycles");
        LOOKUP_MUPENRR_FN(s_mupenrr_shutdown_fn, M64RRSpec::PtrShutdown, "M64RRShutdown");

        g_plugin_funcs.rsp_rom_closed = []() {
            if (s_mupenrr_rom_closed_fn) s_mupenrr_rom_closed_fn();
        };
        g_plugin_funcs.rsp_close_dll = []() {
            if (s_mupenrr_shutdown_fn) s_mupenrr_shutdown_fn();
        };
        g_plugin_funcs.rsp_do_rsp_cycles = [](uint32_t cycles) {
            if (s_mupenrr_do_rsp_cycles_fn)
            {
                s_mupenrr_do_rsp_cycles_fn((uint8_t)cycles);
                return 0u;
            }
            return cycles;
        };

        break;
    }
    default:
        RT_ASSERT(false, L"Unsupported plugin type");
        break;
    }
}

void M64RRPlugin::initiate_dummy()
{
    Main::init_sdl();

    const auto initiate_fn = (M64RRSpec::PtrInitiate)GetProcAddress(m_module, "M64RRInitiate");
    if (!initiate_fn) return;

    M64RRSpec::PluginInit init{};

    init.platform = M64RRSpec::Platform::Windows;
    init.byteswapped = 1;
    init.rom = dummy_header;
    init.rdram = nullptr;
    init.dmem = nullptr;
    init.imem = nullptr;
    init.mi_intr_reg = &dummy_dw;
    init.dpc_start_reg = &dummy_dw;
    init.dpc_end_reg = &dummy_dw;
    init.dpc_current_reg = &dummy_dw;
    init.dpc_status_reg = &dummy_dw;
    init.dpc_clock_reg = &dummy_dw;
    init.dpc_bufbusy_reg = &dummy_dw;
    init.dpc_pipebusy_reg = &dummy_dw;
    init.dpc_tmem_reg = &dummy_dw;
    init.vi_status_reg = &dummy_dw;
    init.vi_origin_reg = &dummy_dw;
    init.vi_width_reg = &dummy_dw;
    init.vi_intr_reg = &dummy_dw;
    init.vi_v_current_line_reg = &dummy_dw;
    init.vi_timing_reg = &dummy_dw;
    init.vi_v_sync_reg = &dummy_dw;
    init.vi_h_sync_reg = &dummy_dw;
    init.vi_leap_reg = &dummy_dw;
    init.vi_h_start_reg = &dummy_dw;
    init.vi_v_start_reg = &dummy_dw;
    init.vi_v_burst_reg = &dummy_dw;
    init.vi_x_scale_reg = &dummy_dw;
    init.vi_y_scale_reg = &dummy_dw;
    init.ai_dram_addr_reg = &dummy_dw;
    init.ai_len_reg = &dummy_dw;
    init.ai_control_reg = &dummy_dw;
    init.ai_status_reg = &dummy_dw;
    init.ai_dacrate_reg = &dummy_dw;
    init.ai_bitrate_reg = &dummy_dw;
    init.sp_mem_addr_reg = &dummy_dw;
    init.sp_dram_addr_reg = &dummy_dw;
    init.sp_rd_len_reg = &dummy_dw;
    init.sp_wr_len_reg = &dummy_dw;
    init.sp_status_reg = &dummy_dw;
    init.sp_dma_full_reg = &dummy_dw;
    init.sp_dma_busy_reg = &dummy_dw;
    init.sp_pc_reg = &dummy_dw;
    init.sp_semaphore_reg = &dummy_dw;

    init.process_dlist = [](const auto &...) {};
    init.header = dummy_header;

    std::array<M64RRSpec::Controller, 4> tmp_controllers{};
    init.controllers = tmp_controllers.data();

    switch (m_type)
    {
    case Plugin::Type::Video:
        g_plugin_funcs.video_extended_funcs = GEN_EXTENDED_FUNCS(g_video_logger);
        init.ef = &g_plugin_funcs.video_extended_funcs;
        break;
    case Plugin::Type::Audio:
        g_plugin_funcs.audio_extended_funcs = GEN_EXTENDED_FUNCS(g_audio_logger);
        init.ef = &g_plugin_funcs.audio_extended_funcs;
        break;
    case Plugin::Type::Input:
        g_plugin_funcs.input_extended_funcs = GEN_EXTENDED_FUNCS(g_input_logger);
        init.ef = &g_plugin_funcs.input_extended_funcs;
        break;
    case Plugin::Type::RSP:
        g_plugin_funcs.rsp_extended_funcs = GEN_EXTENDED_FUNCS(g_rsp_logger);
        init.ef = &g_plugin_funcs.rsp_extended_funcs;
        break;
    }

    initiate_fn(&init);
}

void M64RRPlugin::deinitiate_dummy()
{
    if (g_main_ctx.core_ctx->vr_get_launched()) return;
    const auto close_dll = (M64RRSpec::PtrShutdown)GetProcAddress(m_module, "M64RRShutdown");
    if (close_dll) close_dll();
}

#undef LOOKUP_MUPENRR_FN
