/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// ReSharper disable CppCStyleCast

#include "stdafx.h"
#include <Config.h>
#include <DialogService.h>
#include <Plugin.h>
#include <components/ConfigDialog.h>
#include <components/Statusbar.h>

#define CALL _cdecl

static core_gfx_info dummy_video_info{};
static core_audio_info dummy_audio_info{};
static core_input_info dummy_control_info{};
static core_rsp_info dummy_rsp_info{};
static uint8_t dummy_header[0x40]{};
static uint32_t dummy_dw{};

static core_gfx_info gfx_info{};
static core_audio_info audio_info{};
static core_input_info control_info{};
static core_rsp_info rsp_info{};

static DLLABOUT dll_about{};
static DLLCONFIG dll_config{};
static DLLTEST dll_test{};

static std::shared_ptr<Plugin> video_plugin;
static std::shared_ptr<Plugin> audio_plugin;
static std::shared_ptr<Plugin> input_plugin;
static std::shared_ptr<Plugin> rsp_plugin;

plugin_funcs g_plugin_funcs{};

#pragma region Dummy Functions

static uint32_t CALL dummy_do_rsp_cycles(uint32_t Cycles)
{
    return Cycles;
}

static void CALL dummy_void()
{
}

static void CALL dummy_receive_extended_funcs(core_plugin_extended_funcs *)
{
}

static int32_t CALL dummy_initiate_gfx(core_gfx_info)
{
    return 1;
}

static int32_t CALL dummy_initiate_audio(core_audio_info)
{
    return 1;
}

static void CALL dummy_initiate_controllers(core_input_info)
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

static void CALL dummy_initiate_rsp(core_rsp_info, uint32_t *)
{
}

static void CALL dummy_fb_read(uint32_t)
{
}

static void CALL dummy_fb_write(uint32_t, uint32_t)
{
}

static void CALL dummy_fb_get_framebuffer_info(void *)
{
}

static void CALL dummy_move_screen(int32_t, int32_t)
{
}

#pragma endregion

#define FUNC(target, type, fallback, name)                                                                             \
    target = (type)GetProcAddress((HMODULE)handle, name);                                                              \
    if (!target) target = fallback

/**
 * \brief Tries to find the free function exported by the CRT in the specified module.
 */
static void (*get_free_function_in_module(HMODULE module))(void *)
{
    auto dll_crt_free = (DLLCRTFREE)GetProcAddress(module, "DllCrtFree");
    if (dll_crt_free) return dll_crt_free;

    ULONG size;
    auto import_descriptor = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToDataEx(
        module, true, IMAGE_DIRECTORY_ENTRY_IMPORT, &size, nullptr);
    if (import_descriptor != nullptr)
    {
        while (import_descriptor->Characteristics && import_descriptor->Name)
        {
            auto importDllName = (LPCSTR)((PBYTE)module + import_descriptor->Name);
            auto importDllHandle = GetModuleHandleA(importDllName);
            if (importDllHandle != nullptr)
            {
                dll_crt_free = (DLLCRTFREE)GetProcAddress(importDllHandle, "free");
                if (dll_crt_free != nullptr) return dll_crt_free;
            }

            import_descriptor++;
        }
    }

    return free;
}

void load_gfx(HMODULE handle)
{
    INITIATEGFX initiate_gfx{};

    RECEIVEEXTENDEDFUNCS receive_extended_funcs;
    FUNC(receive_extended_funcs, RECEIVEEXTENDEDFUNCS, dummy_receive_extended_funcs, "ReceiveExtendedFuncs");

    FUNC(g_plugin_funcs.video_change_window, CHANGEWINDOW, dummy_void, "ChangeWindow");
    FUNC(g_plugin_funcs.video_close_dll, CLOSEDLL, dummy_void, "CloseDLL");
    FUNC(initiate_gfx, INITIATEGFX, dummy_initiate_gfx, "InitiateGFX");
    FUNC(g_plugin_funcs.video_process_dlist, PROCESSDLIST, dummy_void, "ProcessDList");
    FUNC(g_plugin_funcs.video_process_rdp_list, PROCESSRDPLIST, dummy_void, "ProcessRDPList");
    FUNC(g_plugin_funcs.video_rom_closed, ROMCLOSED, dummy_void, "RomClosed");
    FUNC(g_plugin_funcs.video_rom_open, ROMOPEN, dummy_void, "RomOpen");
    FUNC(g_plugin_funcs.video_show_cfb, SHOWCFB, dummy_void, "ShowCFB");
    FUNC(g_plugin_funcs.video_update_screen, UPDATESCREEN, dummy_void, "UpdateScreen");
    FUNC(g_plugin_funcs.video_vi_status_changed, VISTATUSCHANGED, dummy_void, "ViStatusChanged");
    FUNC(g_plugin_funcs.video_vi_width_changed, VIWIDTHCHANGED, dummy_void, "ViWidthChanged");
    FUNC(g_plugin_funcs.video_move_screen, MOVESCREEN, dummy_move_screen, "MoveScreen");
    FUNC(g_plugin_funcs.video_capture_screen, CAPTURESCREEN, nullptr, "CaptureScreen");
    FUNC(g_plugin_funcs.video_read_screen, READSCREEN, (READSCREEN)GetProcAddress(handle, "ReadScreen2"), "ReadScreen");
    FUNC(g_plugin_funcs.video_get_video_size, GETVIDEOSIZE, nullptr, "mge_get_video_size");
    FUNC(g_plugin_funcs.video_read_video, READVIDEO, nullptr, "mge_read_video");
    FUNC(g_plugin_funcs.video_fb_read, FBREAD, dummy_fb_read, "FBRead");
    FUNC(g_plugin_funcs.video_fb_write, FBWRITE, dummy_fb_write, "FBWrite");
    FUNC(g_plugin_funcs.video_fb_get_frame_buffer_info, FBGETFRAMEBUFFERINFO, dummy_fb_get_framebuffer_info,
         "FBGetFrameBufferInfo");
    g_plugin_funcs.video_dll_crt_free = get_free_function_in_module(handle);

    gfx_info.main_hwnd = g_main_ctx.hwnd;
    gfx_info.statusbar_hwnd = g_config.is_statusbar_enabled ? Statusbar::hwnd() : nullptr;
    gfx_info.byteswapped = 1;
    gfx_info.rom = g_main_ctx.core_ctx->rom;
    gfx_info.rdram = (uint8_t *)g_main_ctx.core_ctx->rdram;
    gfx_info.dmem = (uint8_t *)g_main_ctx.core_ctx->SP_DMEM;
    gfx_info.imem = (uint8_t *)g_main_ctx.core_ctx->SP_IMEM;
    gfx_info.mi_intr_reg = &g_main_ctx.core_ctx->MI_register->mi_intr_reg;
    gfx_info.dpc_start_reg = &g_main_ctx.core_ctx->dpc_register->dpc_start;
    gfx_info.dpc_end_reg = &g_main_ctx.core_ctx->dpc_register->dpc_end;
    gfx_info.dpc_current_reg = &g_main_ctx.core_ctx->dpc_register->dpc_current;
    gfx_info.dpc_status_reg = &g_main_ctx.core_ctx->dpc_register->dpc_status;
    gfx_info.dpc_clock_reg = &g_main_ctx.core_ctx->dpc_register->dpc_clock;
    gfx_info.dpc_bufbusy_reg = &g_main_ctx.core_ctx->dpc_register->dpc_bufbusy;
    gfx_info.dpc_pipebusy_reg = &g_main_ctx.core_ctx->dpc_register->dpc_pipebusy;
    gfx_info.dpc_tmem_reg = &g_main_ctx.core_ctx->dpc_register->dpc_tmem;
    gfx_info.vi_status_reg = &g_main_ctx.core_ctx->vi_register->vi_status;
    gfx_info.vi_origin_reg = &g_main_ctx.core_ctx->vi_register->vi_origin;
    gfx_info.vi_width_reg = &g_main_ctx.core_ctx->vi_register->vi_width;
    gfx_info.vi_intr_reg = &g_main_ctx.core_ctx->vi_register->vi_v_intr;
    gfx_info.vi_v_current_line_reg = &g_main_ctx.core_ctx->vi_register->vi_current;
    gfx_info.vi_timing_reg = &g_main_ctx.core_ctx->vi_register->vi_burst;
    gfx_info.vi_v_sync_reg = &g_main_ctx.core_ctx->vi_register->vi_v_sync;
    gfx_info.vi_h_sync_reg = &g_main_ctx.core_ctx->vi_register->vi_h_sync;
    gfx_info.vi_leap_reg = &g_main_ctx.core_ctx->vi_register->vi_leap;
    gfx_info.vi_h_start_reg = &g_main_ctx.core_ctx->vi_register->vi_h_start;
    gfx_info.vi_v_start_reg = &g_main_ctx.core_ctx->vi_register->vi_v_start;
    gfx_info.vi_v_burst_reg = &g_main_ctx.core_ctx->vi_register->vi_v_burst;
    gfx_info.vi_x_scale_reg = &g_main_ctx.core_ctx->vi_register->vi_x_scale;
    gfx_info.vi_y_scale_reg = &g_main_ctx.core_ctx->vi_register->vi_y_scale;
    gfx_info.check_interrupts = dummy_void;

    receive_extended_funcs(&g_plugin_funcs.video_extended_funcs);
    initiate_gfx(gfx_info);
}

void load_input(uint16_t version, HMODULE handle)
{
    OLD_INITIATECONTROLLERS old_initiate_controllers{};
    INITIATECONTROLLERS initiate_controllers{};

    RECEIVEEXTENDEDFUNCS receive_extended_funcs;
    FUNC(receive_extended_funcs, RECEIVEEXTENDEDFUNCS, dummy_receive_extended_funcs, "ReceiveExtendedFuncs");

    FUNC(g_plugin_funcs.input_close_dll, CLOSEDLL, dummy_void, "CloseDLL");
    FUNC(g_plugin_funcs.input_controller_command, CONTROLLERCOMMAND, dummy_controller_command, "ControllerCommand");
    FUNC(g_plugin_funcs.input_get_keys, GETKEYS, dummy_get_keys, "GetKeys");
    FUNC(g_plugin_funcs.input_set_keys, SETKEYS, dummy_set_keys, "SetKeys");
    if (version == 0x0101)
    {
        FUNC(initiate_controllers, INITIATECONTROLLERS, dummy_initiate_controllers, "InitiateControllers");
    }
    else
    {
        FUNC(old_initiate_controllers, OLD_INITIATECONTROLLERS, nullptr, "InitiateControllers");
    }
    FUNC(g_plugin_funcs.input_read_controller, READCONTROLLER, dummy_read_controller, "ReadController");
    FUNC(g_plugin_funcs.input_rom_closed, ROMCLOSED, dummy_void, "RomClosed");
    FUNC(g_plugin_funcs.input_rom_open, ROMOPEN, dummy_void, "RomOpen");
    FUNC(g_plugin_funcs.input_key_down, KEYDOWN, dummy_key_down, "WM_KeyDown");
    FUNC(g_plugin_funcs.input_key_up, KEYUP, dummy_key_up, "WM_KeyUp");

    control_info.main_hwnd = g_main_ctx.hwnd;
    control_info.hinst = g_main_ctx.hinst;
    control_info.byteswapped = 1;
    control_info.header = g_main_ctx.core_ctx->rom;
    control_info.controllers = g_main_ctx.core.controls;
    for (auto &controller : g_main_ctx.core.controls)
    {
        controller.Present = 0;
        controller.RawData = 0;
        controller.Plugin = (int32_t)ce_none;
    }
    receive_extended_funcs(&g_plugin_funcs.input_extended_funcs);
    if (version == 0x0101)
    {
        initiate_controllers(control_info);
    }
    else
    {
        old_initiate_controllers(g_main_ctx.hwnd, g_main_ctx.core.controls);
    }
}

void load_audio(HMODULE handle)
{
    INITIATEAUDIO initiate_audio{};

    RECEIVEEXTENDEDFUNCS receive_extended_funcs;
    FUNC(receive_extended_funcs, RECEIVEEXTENDEDFUNCS, dummy_receive_extended_funcs, "ReceiveExtendedFuncs");

    FUNC(g_plugin_funcs.audio_close_dll_audio, CLOSEDLL, dummy_void, "CloseDLL");
    FUNC(g_plugin_funcs.audio_ai_dacrate_changed, AIDACRATECHANGED, dummy_ai_dacrate_changed, "AiDacrateChanged");
    FUNC(g_plugin_funcs.audio_ai_len_changed, AILENCHANGED, dummy_void, "AiLenChanged");
    FUNC(g_plugin_funcs.audio_ai_read_length, AIREADLENGTH, dummy_ai_read_length, "AiReadLength");
    FUNC(initiate_audio, INITIATEAUDIO, dummy_initiate_audio, "InitiateAudio");
    FUNC(g_plugin_funcs.audio_rom_closed, ROMCLOSED, dummy_void, "RomClosed");
    FUNC(g_plugin_funcs.audio_rom_open, ROMOPEN, dummy_void, "RomOpen");
    FUNC(g_plugin_funcs.audio_process_alist, PROCESSALIST, dummy_void, "ProcessAList");
    FUNC(g_plugin_funcs.audio_ai_update, AIUPDATE, dummy_ai_update, "AiUpdate");

    audio_info.main_hwnd = g_main_ctx.hwnd;
    audio_info.hinst = g_main_ctx.hinst;
    audio_info.byteswapped = 1;
    audio_info.rom = g_main_ctx.core_ctx->rom;
    audio_info.rdram = (uint8_t *)g_main_ctx.core_ctx->rdram;
    audio_info.dmem = (uint8_t *)g_main_ctx.core_ctx->SP_DMEM;
    audio_info.imem = (uint8_t *)g_main_ctx.core_ctx->SP_IMEM;
    audio_info.mi_intr_reg = &dummy_dw;
    audio_info.ai_dram_addr_reg = &g_main_ctx.core_ctx->ai_register->ai_dram_addr;
    audio_info.ai_len_reg = &g_main_ctx.core_ctx->ai_register->ai_len;
    audio_info.ai_control_reg = &g_main_ctx.core_ctx->ai_register->ai_control;
    audio_info.ai_status_reg = &dummy_dw;
    audio_info.ai_dacrate_reg = &g_main_ctx.core_ctx->ai_register->ai_dacrate;
    audio_info.ai_bitrate_reg = &g_main_ctx.core_ctx->ai_register->ai_bitrate;

    audio_info.check_interrupts = dummy_void;

    receive_extended_funcs(&g_plugin_funcs.audio_extended_funcs);
    initiate_audio(audio_info);
}

void load_rsp(HMODULE handle)
{
    INITIATERSP initiate_rsp{};

    RECEIVEEXTENDEDFUNCS receive_extended_funcs;
    FUNC(receive_extended_funcs, RECEIVEEXTENDEDFUNCS, dummy_receive_extended_funcs, "ReceiveExtendedFuncs");

    FUNC(g_plugin_funcs.rsp_close_dll, CLOSEDLL, dummy_void, "CloseDLL");
    FUNC(g_plugin_funcs.rsp_do_rsp_cycles, DORSPCYCLES, dummy_do_rsp_cycles, "DoRspCycles");
    FUNC(initiate_rsp, INITIATERSP, dummy_initiate_rsp, "InitiateRSP");
    FUNC(g_plugin_funcs.rsp_rom_closed, ROMCLOSED, dummy_void, "RomClosed");

    rsp_info.byteswapped = 1;
    rsp_info.rdram = (uint8_t *)g_main_ctx.core_ctx->rdram;
    rsp_info.dmem = (uint8_t *)g_main_ctx.core_ctx->SP_DMEM;
    rsp_info.imem = (uint8_t *)g_main_ctx.core_ctx->SP_IMEM;
    rsp_info.mi_intr_reg = &g_main_ctx.core_ctx->MI_register->mi_intr_reg;
    rsp_info.sp_mem_addr_reg = &g_main_ctx.core_ctx->sp_register->sp_mem_addr_reg;
    rsp_info.sp_dram_addr_reg = &g_main_ctx.core_ctx->sp_register->sp_dram_addr_reg;
    rsp_info.sp_rd_len_reg = &g_main_ctx.core_ctx->sp_register->sp_rd_len_reg;
    rsp_info.sp_wr_len_reg = &g_main_ctx.core_ctx->sp_register->sp_wr_len_reg;
    rsp_info.sp_status_reg = &g_main_ctx.core_ctx->sp_register->sp_status_reg;
    rsp_info.sp_dma_full_reg = &g_main_ctx.core_ctx->sp_register->sp_dma_full_reg;
    rsp_info.sp_dma_busy_reg = &g_main_ctx.core_ctx->sp_register->sp_dma_busy_reg;
    rsp_info.sp_pc_reg = &g_main_ctx.core_ctx->rsp_register->rsp_pc;
    rsp_info.sp_semaphore_reg = &g_main_ctx.core_ctx->sp_register->sp_semaphore_reg;
    rsp_info.dpc_start_reg = &g_main_ctx.core_ctx->dpc_register->dpc_start;
    rsp_info.dpc_end_reg = &g_main_ctx.core_ctx->dpc_register->dpc_end;
    rsp_info.dpc_current_reg = &g_main_ctx.core_ctx->dpc_register->dpc_current;
    rsp_info.dpc_status_reg = &g_main_ctx.core_ctx->dpc_register->dpc_status;
    rsp_info.dpc_clock_reg = &g_main_ctx.core_ctx->dpc_register->dpc_clock;
    rsp_info.dpc_bufbusy_reg = &g_main_ctx.core_ctx->dpc_register->dpc_bufbusy;
    rsp_info.dpc_pipebusy_reg = &g_main_ctx.core_ctx->dpc_register->dpc_pipebusy;
    rsp_info.dpc_tmem_reg = &g_main_ctx.core_ctx->dpc_register->dpc_tmem;
    rsp_info.check_interrupts = dummy_void;
    rsp_info.process_dlist_list = g_plugin_funcs.video_process_dlist;
    rsp_info.process_alist_list = g_plugin_funcs.audio_process_alist;
    rsp_info.process_rdp_list = g_plugin_funcs.video_process_rdp_list;
    rsp_info.show_cfb = g_plugin_funcs.video_show_cfb;

    receive_extended_funcs(&g_plugin_funcs.rsp_extended_funcs);

    int32_t i = 4;
    initiate_rsp(rsp_info, (uint32_t *)&i);
}

std::pair<std::wstring, std::unique_ptr<Plugin>> Plugin::create(std::filesystem::path path)
{
    const auto module = LoadLibrary(path.wstring().c_str());
    uint64_t last_error = GetLastError();

    if (module == nullptr)
    {
        return std::make_pair(std::format(L"LoadLibrary (code {})", last_error), nullptr);
    }

    const auto get_dll_info = (GETDLLINFO)GetProcAddress(module, "GetDllInfo");

    if (!get_dll_info)
    {
        if (!FreeLibrary(module))
        {
            DialogService::show_dialog(std::format(L"Failed to free library {:#06x}.", (unsigned long)module).c_str(),
                                       L"Core", fsvc_error);
        }
        return std::make_pair(L"GetDllInfo missing", nullptr);
    }

    core_plugin_info plugin_info;
    get_dll_info(&plugin_info);

    const size_t plugin_name_len = strlen(plugin_info.name);
    while (plugin_info.name[plugin_name_len - 1] == ' ')
    {
        plugin_info.name[plugin_name_len - 1] = '\0';
    }

    auto plugin = std::make_unique<Plugin>();

    plugin->m_path = path;
    plugin->m_name = std::string(plugin_info.name);
    plugin->m_type = static_cast<core_plugin_type>(plugin_info.type);
    plugin->m_version = plugin_info.ver;
    plugin->m_module = module;

    g_view_logger->info("[Plugin] Created plugin {}", plugin->m_name);
    return std::make_pair(L"", std::move(plugin));
}

Plugin::~Plugin()
{
    if (!FreeLibrary(m_module))
    {
        DialogService::show_dialog(std::format(L"Failed to free library {:#06x}.", (unsigned long)m_module).c_str(),
                                   L"Core", fsvc_error);
    }
}

void Plugin::config(const HWND hwnd)
{
    const auto run_config = [&] {
        const auto dll_config = (DLLCONFIG)GetProcAddress(m_module, "DllConfig");

        if (!dll_config)
        {
            DialogService::show_dialog(
                std::format(L"'{}' has no configuration.", IOUtils::to_wide_string(this->name())).c_str(), L"Plugin",
                fsvc_error, hwnd);
            goto cleanup;
        }

        dll_config(hwnd);

    cleanup:

        if (g_main_ctx.core_ctx->vr_get_launched())
        {
            return;
        }

        const auto close_dll = (CLOSEDLL)GetProcAddress(m_module, "CloseDLL");
        if (close_dll) close_dll();
    };

    switch (m_type)
    {
    case plugin_video: {
        if (!g_main_ctx.core_ctx->vr_get_launched())
        {
            // NOTE: Since olden days, dummy render target hwnd was the statusbar.
            dummy_video_info.main_hwnd = Statusbar::hwnd();
            dummy_video_info.statusbar_hwnd = Statusbar::hwnd();

            const auto initiate_gfx = (INITIATEGFX)GetProcAddress(m_module, "InitiateGFX");
            if (initiate_gfx && !initiate_gfx(dummy_video_info))
            {
                DialogService::show_dialog(L"Couldn't initialize video plugin.", L"Core", fsvc_information);
            }
        }

        run_config();

        break;
    }
    case plugin_audio: {
        if (!g_main_ctx.core_ctx->vr_get_launched())
        {
            const auto initiate_audio = (INITIATEAUDIO)GetProcAddress(m_module, "InitiateAudio");
            if (initiate_audio && !initiate_audio(dummy_audio_info))
            {
                DialogService::show_dialog(L"Couldn't initialize audio plugin.", L"Core", fsvc_information);
            }
        }

        run_config();

        break;
    }
    case plugin_input: {
        if (!g_main_ctx.core_ctx->vr_get_launched())
        {
            if (m_version == 0x0101)
            {
                const auto initiate_controllers = (INITIATECONTROLLERS)GetProcAddress(m_module, "InitiateControllers");
                if (initiate_controllers) initiate_controllers(dummy_control_info);
            }
            else
            {
                const auto old_initiate_controllers =
                    (OLD_INITIATECONTROLLERS)GetProcAddress(m_module, "InitiateControllers");
                if (old_initiate_controllers) old_initiate_controllers(g_main_ctx.hwnd, g_main_ctx.core.controls);
            }
        }

        run_config();

        break;
    }
    case plugin_rsp: {
        if (!g_main_ctx.core_ctx->vr_get_launched())
        {
            auto initiateRSP = (INITIATERSP)GetProcAddress(m_module, "InitiateRSP");
            uint32_t i = 0;
            if (initiateRSP) initiateRSP(dummy_rsp_info, &i);
        }

        run_config();

        break;
    }
    default:
        assert(false);
        break;
    }
}

void Plugin::test(const HWND hwnd)
{
    dll_test = (DLLTEST)GetProcAddress(m_module, "DllTest");
    if (dll_test) dll_test(hwnd);
}

void Plugin::about(const HWND hwnd)
{
    dll_about = (DLLABOUT)GetProcAddress(m_module, "DllAbout");
    if (dll_about) dll_about(hwnd);
}

void Plugin::initiate()
{
    switch (m_type)
    {
    case plugin_video:
        load_gfx(m_module);
        break;
    case plugin_audio:
        load_audio(m_module);
        break;
    case plugin_input:
        load_input(m_version, m_module);
        break;
    case plugin_rsp:
        load_rsp(m_module);
        break;
    }
}

t_plugin_discovery_result PluginUtil::discover_plugins(const std::filesystem::path &directory)
{
    std::vector<std::unique_ptr<Plugin>> plugins;
    std::vector<std::pair<std::filesystem::path, std::wstring>> results;

    // this will fail to match files with the extension not lowercased, but I don't think this is a big deal.
    auto dll_files = std::filesystem::directory_iterator(directory) |
                     std::views::filter([](const std::filesystem::directory_entry &entry) {
                         return entry.is_regular_file() && entry.path().extension().compare(L".dll") == 0;
                     }) |
                     std::views::transform([](const std::filesystem::directory_entry &entry) { return entry.path(); });

    for (const auto &file : dll_files)
    {
        auto [result, plugin] = Plugin::create(file);

        results.emplace_back(file, result);
        if (!result.empty()) continue;

        plugins.emplace_back(std::move(plugin));
    }

    return t_plugin_discovery_result{
        .plugins = std::move(plugins),
        .results = results,
    };
}

#define GEN_EXTENDED_FUNCS(logger)                                                                                     \
    core_plugin_extended_funcs                                                                                         \
    {                                                                                                                  \
        .size = sizeof(core_plugin_extended_funcs), .log_trace = [](const wchar_t *str) { logger->trace(str); },       \
        .log_info = [](const wchar_t *str) { logger->info(str); },                                                     \
        .log_warn = [](const wchar_t *str) { logger->warn(str); },                                                     \
        .log_error = [](const wchar_t *str) { logger->error(str); },                                                   \
    }

void PluginUtil::init_dummy_and_extended_funcs()
{
    dummy_video_info.byteswapped = 1;
    dummy_video_info.rom = (uint8_t *)dummy_header;
    dummy_video_info.rdram = (uint8_t *)g_main_ctx.core_ctx->rdram;
    dummy_video_info.dmem = (uint8_t *)g_main_ctx.core_ctx->SP_DMEM;
    dummy_video_info.imem = (uint8_t *)g_main_ctx.core_ctx->SP_IMEM;
    dummy_video_info.mi_intr_reg = &g_main_ctx.core_ctx->MI_register->mi_intr_reg;
    dummy_video_info.dpc_start_reg = &g_main_ctx.core_ctx->dpc_register->dpc_start;
    dummy_video_info.dpc_end_reg = &g_main_ctx.core_ctx->dpc_register->dpc_end;
    dummy_video_info.dpc_current_reg = &g_main_ctx.core_ctx->dpc_register->dpc_current;
    dummy_video_info.dpc_status_reg = &g_main_ctx.core_ctx->dpc_register->dpc_status;
    dummy_video_info.dpc_clock_reg = &g_main_ctx.core_ctx->dpc_register->dpc_clock;
    dummy_video_info.dpc_bufbusy_reg = &g_main_ctx.core_ctx->dpc_register->dpc_bufbusy;
    dummy_video_info.dpc_pipebusy_reg = &g_main_ctx.core_ctx->dpc_register->dpc_pipebusy;
    dummy_video_info.dpc_tmem_reg = &g_main_ctx.core_ctx->dpc_register->dpc_tmem;
    dummy_video_info.vi_status_reg = &g_main_ctx.core_ctx->vi_register->vi_status;
    dummy_video_info.vi_origin_reg = &g_main_ctx.core_ctx->vi_register->vi_origin;
    dummy_video_info.vi_width_reg = &g_main_ctx.core_ctx->vi_register->vi_width;
    dummy_video_info.vi_intr_reg = &g_main_ctx.core_ctx->vi_register->vi_v_intr;
    dummy_video_info.vi_v_current_line_reg = &g_main_ctx.core_ctx->vi_register->vi_current;
    dummy_video_info.vi_timing_reg = &g_main_ctx.core_ctx->vi_register->vi_burst;
    dummy_video_info.vi_v_sync_reg = &g_main_ctx.core_ctx->vi_register->vi_v_sync;
    dummy_video_info.vi_h_sync_reg = &g_main_ctx.core_ctx->vi_register->vi_h_sync;
    dummy_video_info.vi_leap_reg = &g_main_ctx.core_ctx->vi_register->vi_leap;
    dummy_video_info.vi_h_start_reg = &g_main_ctx.core_ctx->vi_register->vi_h_start;
    dummy_video_info.vi_v_start_reg = &g_main_ctx.core_ctx->vi_register->vi_v_start;
    dummy_video_info.vi_v_burst_reg = &g_main_ctx.core_ctx->vi_register->vi_v_burst;
    dummy_video_info.vi_x_scale_reg = &g_main_ctx.core_ctx->vi_register->vi_x_scale;
    dummy_video_info.vi_y_scale_reg = &g_main_ctx.core_ctx->vi_register->vi_y_scale;
    dummy_video_info.check_interrupts = dummy_void;

    dummy_audio_info.main_hwnd = g_main_ctx.hwnd;
    dummy_audio_info.hinst = g_main_ctx.hinst;
    dummy_audio_info.byteswapped = 1;
    dummy_audio_info.rom = (uint8_t *)dummy_header;
    dummy_audio_info.rdram = (uint8_t *)g_main_ctx.core_ctx->rdram;
    dummy_audio_info.dmem = (uint8_t *)g_main_ctx.core_ctx->SP_DMEM;
    dummy_audio_info.imem = (uint8_t *)g_main_ctx.core_ctx->SP_IMEM;
    dummy_audio_info.mi_intr_reg = &g_main_ctx.core_ctx->MI_register->mi_intr_reg;
    dummy_audio_info.ai_dram_addr_reg = &g_main_ctx.core_ctx->ai_register->ai_dram_addr;
    dummy_audio_info.ai_len_reg = &g_main_ctx.core_ctx->ai_register->ai_len;
    dummy_audio_info.ai_control_reg = &g_main_ctx.core_ctx->ai_register->ai_control;
    dummy_audio_info.ai_status_reg = &g_main_ctx.core_ctx->ai_register->ai_status;
    dummy_audio_info.ai_dacrate_reg = &g_main_ctx.core_ctx->ai_register->ai_dacrate;
    dummy_audio_info.ai_bitrate_reg = &g_main_ctx.core_ctx->ai_register->ai_bitrate;
    dummy_audio_info.check_interrupts = dummy_void;

    dummy_control_info.main_hwnd = g_main_ctx.hwnd;
    dummy_control_info.hinst = g_main_ctx.hinst;
    dummy_control_info.byteswapped = 1;
    dummy_control_info.header = (uint8_t *)dummy_header;
    dummy_control_info.controllers = g_main_ctx.core.controls;
    for (int32_t i = 0; i < 4; i++)
    {
        g_main_ctx.core.controls[i].Present = 0;
        g_main_ctx.core.controls[i].RawData = 0;
        g_main_ctx.core.controls[i].Plugin = (int32_t)ce_none;
    }

    dummy_rsp_info.byteswapped = 1;
    dummy_rsp_info.rdram = (uint8_t *)g_main_ctx.core_ctx->rdram;
    dummy_rsp_info.dmem = (uint8_t *)g_main_ctx.core_ctx->SP_DMEM;
    dummy_rsp_info.imem = (uint8_t *)g_main_ctx.core_ctx->SP_IMEM;
    dummy_rsp_info.mi_intr_reg = &g_main_ctx.core_ctx->MI_register->mi_intr_reg;
    dummy_rsp_info.sp_mem_addr_reg = &g_main_ctx.core_ctx->sp_register->sp_mem_addr_reg;
    dummy_rsp_info.sp_dram_addr_reg = &g_main_ctx.core_ctx->sp_register->sp_dram_addr_reg;
    dummy_rsp_info.sp_rd_len_reg = &g_main_ctx.core_ctx->sp_register->sp_rd_len_reg;
    dummy_rsp_info.sp_wr_len_reg = &g_main_ctx.core_ctx->sp_register->sp_wr_len_reg;
    dummy_rsp_info.sp_status_reg = &g_main_ctx.core_ctx->sp_register->sp_status_reg;
    dummy_rsp_info.sp_dma_full_reg = &g_main_ctx.core_ctx->sp_register->sp_dma_full_reg;
    dummy_rsp_info.sp_dma_busy_reg = &g_main_ctx.core_ctx->sp_register->sp_dma_busy_reg;
    dummy_rsp_info.sp_pc_reg = &g_main_ctx.core_ctx->rsp_register->rsp_pc;
    dummy_rsp_info.sp_semaphore_reg = &g_main_ctx.core_ctx->sp_register->sp_semaphore_reg;
    dummy_rsp_info.dpc_start_reg = &g_main_ctx.core_ctx->dpc_register->dpc_start;
    dummy_rsp_info.dpc_end_reg = &g_main_ctx.core_ctx->dpc_register->dpc_end;
    dummy_rsp_info.dpc_current_reg = &g_main_ctx.core_ctx->dpc_register->dpc_current;
    dummy_rsp_info.dpc_status_reg = &g_main_ctx.core_ctx->dpc_register->dpc_status;
    dummy_rsp_info.dpc_clock_reg = &g_main_ctx.core_ctx->dpc_register->dpc_clock;
    dummy_rsp_info.dpc_bufbusy_reg = &g_main_ctx.core_ctx->dpc_register->dpc_bufbusy;
    dummy_rsp_info.dpc_pipebusy_reg = &g_main_ctx.core_ctx->dpc_register->dpc_pipebusy;
    dummy_rsp_info.dpc_tmem_reg = &g_main_ctx.core_ctx->dpc_register->dpc_tmem;
    dummy_rsp_info.check_interrupts = dummy_void;
    dummy_rsp_info.process_dlist_list = g_plugin_funcs.video_process_dlist;
    dummy_rsp_info.process_alist_list = g_plugin_funcs.audio_process_alist;
    dummy_rsp_info.process_rdp_list = g_plugin_funcs.video_process_rdp_list;
    dummy_rsp_info.show_cfb = g_plugin_funcs.video_show_cfb;

    g_plugin_funcs.video_extended_funcs = GEN_EXTENDED_FUNCS(g_video_logger);
    g_plugin_funcs.audio_extended_funcs = GEN_EXTENDED_FUNCS(g_audio_logger);
    g_plugin_funcs.input_extended_funcs = GEN_EXTENDED_FUNCS(g_input_logger);
    g_plugin_funcs.rsp_extended_funcs = GEN_EXTENDED_FUNCS(g_rsp_logger);
}

bool PluginUtil::mge_available()
{
    return g_plugin_funcs.video_read_video && g_plugin_funcs.video_get_video_size;
}

void PluginUtil::start_plugins()
{
    g_main_ctx.core.video_process_dlist = g_plugin_funcs.video_process_dlist;
    g_main_ctx.core.video_process_rdp_list = g_plugin_funcs.video_process_rdp_list;
    g_main_ctx.core.video_show_cfb = g_plugin_funcs.video_show_cfb;
    g_main_ctx.core.video_vi_status_changed = g_plugin_funcs.video_vi_status_changed;
    g_main_ctx.core.video_vi_width_changed = g_plugin_funcs.video_vi_width_changed;
    g_main_ctx.core.video_get_video_size = g_plugin_funcs.video_get_video_size;
    g_main_ctx.core.video_fb_read = g_plugin_funcs.video_fb_read;
    g_main_ctx.core.video_fb_write = g_plugin_funcs.video_fb_write;
    g_main_ctx.core.video_fb_get_frame_buffer_info = g_plugin_funcs.video_fb_get_frame_buffer_info;

    g_main_ctx.core.audio_ai_dacrate_changed = g_plugin_funcs.audio_ai_dacrate_changed;
    g_main_ctx.core.audio_ai_len_changed = g_plugin_funcs.audio_ai_len_changed;
    g_main_ctx.core.audio_ai_read_length = g_plugin_funcs.audio_ai_read_length;
    g_main_ctx.core.audio_process_alist = g_plugin_funcs.audio_process_alist;
    g_main_ctx.core.audio_ai_update = g_plugin_funcs.audio_ai_update;

    g_main_ctx.core.input_controller_command = g_plugin_funcs.input_controller_command;
    g_main_ctx.core.input_get_keys = g_plugin_funcs.input_get_keys;
    g_main_ctx.core.input_set_keys = g_plugin_funcs.input_set_keys;
    g_main_ctx.core.input_read_controller = g_plugin_funcs.input_read_controller;

    g_main_ctx.core.rsp_do_rsp_cycles = g_plugin_funcs.rsp_do_rsp_cycles;

    g_plugin_funcs.video_rom_open();
    g_plugin_funcs.input_rom_open();
    g_plugin_funcs.audio_rom_open();
}

void PluginUtil::stop_plugins()
{
    g_plugin_funcs.video_rom_closed();
    g_plugin_funcs.audio_rom_closed();
    g_plugin_funcs.input_rom_closed();
    g_plugin_funcs.rsp_rom_closed();
    g_plugin_funcs.video_close_dll();
    g_plugin_funcs.audio_close_dll_audio();
    g_plugin_funcs.input_close_dll();
    g_plugin_funcs.rsp_close_dll();
}
bool PluginUtil::load_plugins()
{
    if (video_plugin.get() && audio_plugin.get() && input_plugin.get() && rsp_plugin.get() &&
        video_plugin->path() == g_config.selected_video_plugin &&
        audio_plugin->path() == g_config.selected_audio_plugin &&
        input_plugin->path() == g_config.selected_input_plugin && rsp_plugin->path() == g_config.selected_rsp_plugin)
    {
        g_core_logger->info("[Core] Plugins unchanged, reusing...");
    }
    else
    {
        video_plugin.reset();
        audio_plugin.reset();
        input_plugin.reset();
        rsp_plugin.reset();

        g_view_logger->trace(L"Loading video plugin: {}", g_config.selected_video_plugin);
        g_view_logger->trace(L"Loading audio plugin: {}", g_config.selected_audio_plugin);
        g_view_logger->trace(L"Loading input plugin: {}", g_config.selected_input_plugin);
        g_view_logger->trace(L"Loading RSP plugin: {}", g_config.selected_rsp_plugin);

        auto video_pl = Plugin::create(g_config.selected_video_plugin);
        auto audio_pl = Plugin::create(g_config.selected_audio_plugin);
        auto input_pl = Plugin::create(g_config.selected_input_plugin);
        auto rsp_pl = Plugin::create(g_config.selected_rsp_plugin);

        if (!video_pl.first.empty())
        {
            g_view_logger->error(L"Failed to load video plugin: {}", video_pl.first);
        }
        if (!audio_pl.first.empty())
        {
            g_view_logger->error(L"Failed to load audio plugin: {}", audio_pl.first);
        }
        if (!input_pl.first.empty())
        {
            g_view_logger->error(L"Failed to load input plugin: {}", input_pl.first);
        }
        if (!rsp_pl.first.empty())
        {
            g_view_logger->error(L"Failed to load rsp plugin: {}", rsp_pl.first);
        }

        if (video_pl.second == nullptr || audio_pl.second == nullptr || input_pl.second == nullptr ||
            rsp_pl.second == nullptr)
        {
            video_pl.second.reset();
            audio_pl.second.reset();
            input_pl.second.reset();
            rsp_pl.second.reset();
            return false;
        }

        video_plugin = std::move(video_pl.second);
        audio_plugin = std::move(audio_pl.second);
        input_plugin = std::move(input_pl.second);
        rsp_plugin = std::move(rsp_pl.second);
    }
    return true;
}

void PluginUtil::initiate_plugins()
{
    // HACK: We sleep between each plugin load, as that seems to remedy various plugins failing to initialize correctly.
    auto gfx_plugin_thread = std::thread([] { video_plugin->initiate(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto audio_plugin_thread = std::thread([] { audio_plugin->initiate(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto input_plugin_thread = std::thread([] { input_plugin->initiate(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto rsp_plugin_thread = std::thread([] { rsp_plugin->initiate(); });

    gfx_plugin_thread.join();
    audio_plugin_thread.join();
    input_plugin_thread.join();
    rsp_plugin_thread.join();
}

void PluginUtil::get_plugin_names(char *video, char *audio, char *input, char *rsp)
{
    const auto copy = [&](const std::shared_ptr<Plugin> &plugin, char *type) {
        RT_ASSERT(plugin.get(), L"Plugin not loaded");
        const auto result = strncpy_s(type, 64 - 1, plugin->name().c_str(), plugin->name().size());
        RT_ASSERT(!result, L"Plugin name copy failed");
    };

    copy(video_plugin, video);
    copy(audio_plugin, audio);
    copy(input_plugin, input);
    copy(rsp_plugin, rsp);
}
