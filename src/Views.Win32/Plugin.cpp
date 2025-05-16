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

static core_gfx_info dummy_gfx_info{};
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

#pragma region Dummy Functions

static uint32_t CALL dummy_do_rsp_cycles(uint32_t Cycles)
{
    return Cycles;
}

static void CALL dummy_void()
{
}

static void CALL dummy_receive_extended_funcs(core_plugin_extended_funcs*)
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

static void CALL dummy_controller_command(int32_t, uint8_t*)
{
}

static void CALL dummy_get_keys(int32_t, core_buttons*)
{
}

static void CALL dummy_set_keys(int32_t, core_buttons)
{
}

static void CALL dummy_read_controller(int32_t, uint8_t*)
{
}

static void CALL dummy_key_down(uint32_t, int32_t)
{
}

static void CALL dummy_key_up(uint32_t, int32_t)
{
}

static void CALL dummy_initiate_rsp(core_rsp_info, uint32_t*)
{
}

static void CALL dummy_fb_read(uint32_t)
{
}

static void CALL dummy_fb_write(uint32_t, uint32_t)
{
}

static void CALL dummy_fb_get_framebuffer_info(void*)
{
}

static void CALL dummy_move_screen(int32_t, int32_t)
{
}

#pragma endregion

#define FUNC(target, type, fallback, name)                \
    target = (type)GetProcAddress((HMODULE)handle, name); \
    if (!target)                                          \
    target = fallback

/**
 * \brief Tries to find the free function exported by the CRT in the specified module.
 */
static void (*get_free_function_in_module(HMODULE module))(void*)
{
    auto dll_crt_free = (DLLCRTFREE)GetProcAddress(module, "DllCrtFree");
    if (dll_crt_free)
        return dll_crt_free;

    ULONG size;
    auto import_descriptor = (PIMAGE_IMPORT_DESCRIPTOR)
    ImageDirectoryEntryToDataEx(module, true, IMAGE_DIRECTORY_ENTRY_IMPORT, &size, nullptr);
    if (import_descriptor != nullptr)
    {
        while (import_descriptor->Characteristics && import_descriptor->Name)
        {
            auto importDllName = (LPCSTR)((PBYTE)module + import_descriptor->Name);
            auto importDllHandle = GetModuleHandleA(importDllName);
            if (importDllHandle != nullptr)
            {
                dll_crt_free = (DLLCRTFREE)GetProcAddress(importDllHandle, "free");
                if (dll_crt_free != nullptr)
                    return dll_crt_free;
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

    FUNC(g_core.plugin_funcs.video_change_window, CHANGEWINDOW, dummy_void, "ChangeWindow");
    FUNC(g_core.plugin_funcs.video_close_dll, CLOSEDLL, dummy_void, "CloseDLL");
    FUNC(initiate_gfx, INITIATEGFX, dummy_initiate_gfx, "InitiateGFX");
    FUNC(g_core.plugin_funcs.video_process_dlist, PROCESSDLIST, dummy_void, "ProcessDList");
    FUNC(g_core.plugin_funcs.video_process_rdp_list, PROCESSRDPLIST, dummy_void, "ProcessRDPList");
    FUNC(g_core.plugin_funcs.video_rom_closed, ROMCLOSED, dummy_void, "RomClosed");
    FUNC(g_core.plugin_funcs.video_rom_open, ROMOPEN, dummy_void, "RomOpen");
    FUNC(g_core.plugin_funcs.video_show_cfb, SHOWCFB, dummy_void, "ShowCFB");
    FUNC(g_core.plugin_funcs.video_update_screen, UPDATESCREEN, dummy_void, "UpdateScreen");
    FUNC(g_core.plugin_funcs.video_vi_status_changed, VISTATUSCHANGED, dummy_void, "ViStatusChanged");
    FUNC(g_core.plugin_funcs.video_vi_width_changed, VIWIDTHCHANGED, dummy_void, "ViWidthChanged");
    FUNC(g_core.plugin_funcs.video_move_screen, MOVESCREEN, dummy_move_screen, "MoveScreen");
    FUNC(g_core.plugin_funcs.video_capture_screen, CAPTURESCREEN, nullptr, "CaptureScreen");
    FUNC(g_core.plugin_funcs.video_read_screen, READSCREEN, (READSCREEN)GetProcAddress(handle, "ReadScreen2"), "ReadScreen");
    FUNC(g_core.plugin_funcs.video_get_video_size, GETVIDEOSIZE, nullptr, "mge_get_video_size");
    FUNC(g_core.plugin_funcs.video_read_video, READVIDEO, nullptr, "mge_read_video");
    FUNC(g_core.plugin_funcs.video_fb_read, FBREAD, dummy_fb_read, "FBRead");
    FUNC(g_core.plugin_funcs.video_fb_write, FBWRITE, dummy_fb_write, "FBWrite");
    FUNC(g_core.plugin_funcs.video_fb_get_frame_buffer_info, FBGETFRAMEBUFFERINFO, dummy_fb_get_framebuffer_info, "FBGetFrameBufferInfo");
    g_core.plugin_funcs.video_dll_crt_free = get_free_function_in_module(handle);

    gfx_info.main_hwnd = g_main_hwnd;
    gfx_info.statusbar_hwnd = g_config.is_statusbar_enabled ? Statusbar::hwnd() : nullptr;
    gfx_info.byteswapped = 1;
    gfx_info.rom = g_core.rom;
    gfx_info.rdram = (uint8_t*)g_core.rdram;
    gfx_info.dmem = (uint8_t*)g_core.SP_DMEM;
    gfx_info.imem = (uint8_t*)g_core.SP_IMEM;
    gfx_info.mi_intr_reg = &g_core.MI_register->mi_intr_reg;
    gfx_info.dpc_start_reg = &g_core.dpc_register->dpc_start;
    gfx_info.dpc_end_reg = &g_core.dpc_register->dpc_end;
    gfx_info.dpc_current_reg = &g_core.dpc_register->dpc_current;
    gfx_info.dpc_status_reg = &g_core.dpc_register->dpc_status;
    gfx_info.dpc_clock_reg = &g_core.dpc_register->dpc_clock;
    gfx_info.dpc_bufbusy_reg = &g_core.dpc_register->dpc_bufbusy;
    gfx_info.dpc_pipebusy_reg = &g_core.dpc_register->dpc_pipebusy;
    gfx_info.dpc_tmem_reg = &g_core.dpc_register->dpc_tmem;
    gfx_info.vi_status_reg = &g_core.vi_register->vi_status;
    gfx_info.vi_origin_reg = &g_core.vi_register->vi_origin;
    gfx_info.vi_width_reg = &g_core.vi_register->vi_width;
    gfx_info.vi_intr_reg = &g_core.vi_register->vi_v_intr;
    gfx_info.vi_v_current_line_reg = &g_core.vi_register->vi_current;
    gfx_info.vi_timing_reg = &g_core.vi_register->vi_burst;
    gfx_info.vi_v_sync_reg = &g_core.vi_register->vi_v_sync;
    gfx_info.vi_h_sync_reg = &g_core.vi_register->vi_h_sync;
    gfx_info.vi_leap_reg = &g_core.vi_register->vi_leap;
    gfx_info.vi_h_start_reg = &g_core.vi_register->vi_h_start;
    gfx_info.vi_v_start_reg = &g_core.vi_register->vi_v_start;
    gfx_info.vi_v_burst_reg = &g_core.vi_register->vi_v_burst;
    gfx_info.vi_x_scale_reg = &g_core.vi_register->vi_x_scale;
    gfx_info.vi_y_scale_reg = &g_core.vi_register->vi_y_scale;
    gfx_info.check_interrupts = dummy_void;

    receive_extended_funcs(&g_core.plugin_funcs.video_extended_funcs);
    initiate_gfx(gfx_info);
}

void load_input(uint16_t version, HMODULE handle)
{
    OLD_INITIATECONTROLLERS old_initiate_controllers{};
    INITIATECONTROLLERS initiate_controllers{};

    RECEIVEEXTENDEDFUNCS receive_extended_funcs;
    FUNC(receive_extended_funcs, RECEIVEEXTENDEDFUNCS, dummy_receive_extended_funcs, "ReceiveExtendedFuncs");

    FUNC(g_core.plugin_funcs.input_close_dll, CLOSEDLL, dummy_void, "CloseDLL");
    FUNC(g_core.plugin_funcs.input_controller_command, CONTROLLERCOMMAND, dummy_controller_command, "ControllerCommand");
    FUNC(g_core.plugin_funcs.input_get_keys, GETKEYS, dummy_get_keys, "GetKeys");
    FUNC(g_core.plugin_funcs.input_set_keys, SETKEYS, dummy_set_keys, "SetKeys");
    if (version == 0x0101)
    {
        FUNC(initiate_controllers, INITIATECONTROLLERS, dummy_initiate_controllers, "InitiateControllers");
    }
    else
    {
        FUNC(old_initiate_controllers, OLD_INITIATECONTROLLERS, nullptr, "InitiateControllers");
    }
    FUNC(g_core.plugin_funcs.input_read_controller, READCONTROLLER, dummy_read_controller, "ReadController");
    FUNC(g_core.plugin_funcs.input_rom_closed, ROMCLOSED, dummy_void, "RomClosed");
    FUNC(g_core.plugin_funcs.input_rom_open, ROMOPEN, dummy_void, "RomOpen");
    FUNC(g_core.plugin_funcs.input_key_down, KEYDOWN, dummy_key_down, "WM_KeyDown");
    FUNC(g_core.plugin_funcs.input_key_up, KEYUP, dummy_key_up, "WM_KeyUp");

    control_info.main_hwnd = g_main_hwnd;
    control_info.hinst = g_app_instance;
    control_info.byteswapped = 1;
    control_info.header = g_core.rom;
    control_info.controllers = g_core.controls;
    for (auto& controller : g_core.controls)
    {
        controller.Present = 0;
        controller.RawData = 0;
        controller.Plugin = (int32_t)ce_none;
    }
    receive_extended_funcs(&g_core.plugin_funcs.input_extended_funcs);
    if (version == 0x0101)
    {
        initiate_controllers(control_info);
    }
    else
    {
        old_initiate_controllers(g_main_hwnd, g_core.controls);
    }
}


void load_audio(HMODULE handle)
{
    INITIATEAUDIO initiate_audio{};

    RECEIVEEXTENDEDFUNCS receive_extended_funcs;
    FUNC(receive_extended_funcs, RECEIVEEXTENDEDFUNCS, dummy_receive_extended_funcs, "ReceiveExtendedFuncs");

    FUNC(g_core.plugin_funcs.audio_close_dll_audio, CLOSEDLL, dummy_void, "CloseDLL");
    FUNC(g_core.plugin_funcs.audio_ai_dacrate_changed, AIDACRATECHANGED, dummy_ai_dacrate_changed, "AiDacrateChanged");
    FUNC(g_core.plugin_funcs.audio_ai_len_changed, AILENCHANGED, dummy_void, "AiLenChanged");
    FUNC(g_core.plugin_funcs.audio_ai_read_length, AIREADLENGTH, dummy_ai_read_length, "AiReadLength");
    FUNC(initiate_audio, INITIATEAUDIO, dummy_initiate_audio, "InitiateAudio");
    FUNC(g_core.plugin_funcs.audio_rom_closed, ROMCLOSED, dummy_void, "RomClosed");
    FUNC(g_core.plugin_funcs.audio_rom_open, ROMOPEN, dummy_void, "RomOpen");
    FUNC(g_core.plugin_funcs.audio_process_alist, PROCESSALIST, dummy_void, "ProcessAList");
    FUNC(g_core.plugin_funcs.audio_ai_update, AIUPDATE, dummy_ai_update, "AiUpdate");

    audio_info.main_hwnd = g_main_hwnd;
    audio_info.hinst = g_app_instance;
    audio_info.byteswapped = 1;
    audio_info.rom = g_core.rom;
    audio_info.rdram = (uint8_t*)g_core.rdram;
    audio_info.dmem = (uint8_t*)g_core.SP_DMEM;
    audio_info.imem = (uint8_t*)g_core.SP_IMEM;
    audio_info.mi_intr_reg = &dummy_dw;
    audio_info.ai_dram_addr_reg = &g_core.ai_register->ai_dram_addr;
    audio_info.ai_len_reg = &g_core.ai_register->ai_len;
    audio_info.ai_control_reg = &g_core.ai_register->ai_control;
    audio_info.ai_status_reg = &dummy_dw;
    audio_info.ai_dacrate_reg = &g_core.ai_register->ai_dacrate;
    audio_info.ai_bitrate_reg = &g_core.ai_register->ai_bitrate;

    audio_info.check_interrupts = dummy_void;

    receive_extended_funcs(&g_core.plugin_funcs.audio_extended_funcs);
    initiate_audio(audio_info);
}

void load_rsp(HMODULE handle)
{
    INITIATERSP initiate_rsp{};

    RECEIVEEXTENDEDFUNCS receive_extended_funcs;
    FUNC(receive_extended_funcs, RECEIVEEXTENDEDFUNCS, dummy_receive_extended_funcs, "ReceiveExtendedFuncs");

    FUNC(g_core.plugin_funcs.rsp_close_dll, CLOSEDLL, dummy_void, "CloseDLL");
    FUNC(g_core.plugin_funcs.rsp_do_rsp_cycles, DORSPCYCLES, dummy_do_rsp_cycles, "DoRspCycles");
    FUNC(initiate_rsp, INITIATERSP, dummy_initiate_rsp, "InitiateRSP");
    FUNC(g_core.plugin_funcs.rsp_rom_closed, ROMCLOSED, dummy_void, "RomClosed");

    rsp_info.byteswapped = 1;
    rsp_info.rdram = (uint8_t*)g_core.rdram;
    rsp_info.dmem = (uint8_t*)g_core.SP_DMEM;
    rsp_info.imem = (uint8_t*)g_core.SP_IMEM;
    rsp_info.mi_intr_reg = &g_core.MI_register->mi_intr_reg;
    rsp_info.sp_mem_addr_reg = &g_core.sp_register->sp_mem_addr_reg;
    rsp_info.sp_dram_addr_reg = &g_core.sp_register->sp_dram_addr_reg;
    rsp_info.sp_rd_len_reg = &g_core.sp_register->sp_rd_len_reg;
    rsp_info.sp_wr_len_reg = &g_core.sp_register->sp_wr_len_reg;
    rsp_info.sp_status_reg = &g_core.sp_register->sp_status_reg;
    rsp_info.sp_dma_full_reg = &g_core.sp_register->sp_dma_full_reg;
    rsp_info.sp_dma_busy_reg = &g_core.sp_register->sp_dma_busy_reg;
    rsp_info.sp_pc_reg = &g_core.rsp_register->rsp_pc;
    rsp_info.sp_semaphore_reg = &g_core.sp_register->sp_semaphore_reg;
    rsp_info.dpc_start_reg = &g_core.dpc_register->dpc_start;
    rsp_info.dpc_end_reg = &g_core.dpc_register->dpc_end;
    rsp_info.dpc_current_reg = &g_core.dpc_register->dpc_current;
    rsp_info.dpc_status_reg = &g_core.dpc_register->dpc_status;
    rsp_info.dpc_clock_reg = &g_core.dpc_register->dpc_clock;
    rsp_info.dpc_bufbusy_reg = &g_core.dpc_register->dpc_bufbusy;
    rsp_info.dpc_pipebusy_reg = &g_core.dpc_register->dpc_pipebusy;
    rsp_info.dpc_tmem_reg = &g_core.dpc_register->dpc_tmem;
    rsp_info.check_interrupts = dummy_void;
    rsp_info.process_dlist_list = g_core.plugin_funcs.video_process_dlist;
    rsp_info.process_alist_list = g_core.plugin_funcs.audio_process_alist;
    rsp_info.process_rdp_list = g_core.plugin_funcs.video_process_rdp_list;
    rsp_info.show_cfb = g_core.plugin_funcs.video_show_cfb;

    receive_extended_funcs(&g_core.plugin_funcs.rsp_extended_funcs);

    int32_t i = 4;
    initiate_rsp(rsp_info, (uint32_t*)&i);
}

std::pair<std::wstring, std::unique_ptr<Plugin>> Plugin::create(std::filesystem::path path)
{
    const auto module = LoadLibrary(path.wstring().c_str());
    uint64_t last_error = GetLastError();

    if (module == nullptr)
    {
        return std::make_pair(std::format(L"LoadLibrary (code {})", last_error), nullptr);
    }

    const auto get_dll_info = (GETDLLINFO)GetProcAddress(
    module,
    "GetDllInfo");

    if (!get_dll_info)
    {
        if (!FreeLibrary(module))
        {
            DialogService::show_dialog(std::format(L"Failed to free library {:#06x}.", (unsigned long)module).c_str(), L"Core", fsvc_error);
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
        DialogService::show_dialog(std::format(L"Failed to free library {:#06x}.", (unsigned long)m_module).c_str(), L"Core", fsvc_error);
    }
}

void Plugin::config()
{
    const auto run_config = [&] {
        const auto dll_config = (DLLCONFIG)GetProcAddress(m_module, "DllConfig");
        // const auto get_config_1 = (GETCONFIG1)GetProcAddress(m_module, "GetConfig1");
        // const auto save_config_1 = (SAVECONFIG1)GetProcAddress(m_module, "SaveConfig1");

        if (!dll_config)
        {
            DialogService::show_dialog(std::format(L"'{}' has no configuration.", string_to_wstring(this->name())).c_str(), L"Plugin", fsvc_error, g_hwnd_plug);
            goto cleanup;
        }
        
        // if (!get_config_1 && !dll_config || (get_config_1 && !save_config_1))
        // {
        //     DialogService::show_dialog(std::format(L"'{}' has no configuration.", string_to_wstring(this->name())).c_str(), L"Plugin", fsvc_error, g_hwnd_plug);
        //     goto cleanup;
        // }
        //
        // if (get_config_1)
        // {
        //     core_plugin_cfg* cfg;
        //     get_config_1(&cfg);
        //     if (cfg)
        //     {
        //         const bool save = ConfigDialog::show_plugin_settings(this, cfg);
        //         if (save && !save_config_1())
        //         {
        //             DialogService::show_dialog(L"Couldn't save plugin configuration.", L"Plugin", fsvc_error, g_hwnd_plug);
        //         }
        //         goto cleanup;
        //     }
        // }

        dll_config(g_hwnd_plug);

    cleanup:

        if (core_vr_get_launched())
        {
            return;
        }

        const auto close_dll = (CLOSEDLL)GetProcAddress(m_module, "CloseDLL");
        if (close_dll)
            close_dll();
    };

    switch (m_type)
    {
    case plugin_video:
        {
            if (!core_vr_get_launched())
            {
                // NOTE: Since olden days, dummy render target hwnd was the statusbar.
                dummy_gfx_info.main_hwnd = Statusbar::hwnd();
                dummy_gfx_info.statusbar_hwnd = Statusbar::hwnd();

                const auto initiate_gfx = (INITIATEGFX)GetProcAddress(m_module, "InitiateGFX");
                if (initiate_gfx && !initiate_gfx(dummy_gfx_info))
                {
                    DialogService::show_dialog(L"Couldn't initialize video plugin.", L"Core", fsvc_information);
                }
            }

            run_config();

            break;
        }
    case plugin_audio:
        {
            if (!core_vr_get_launched())
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
    case plugin_input:
        {
            if (!core_vr_get_launched())
            {
                if (m_version == 0x0101)
                {
                    const auto initiate_controllers = (INITIATECONTROLLERS)GetProcAddress(m_module, "InitiateControllers");
                    if (initiate_controllers)
                        initiate_controllers(dummy_control_info);
                }
                else
                {
                    const auto old_initiate_controllers = (OLD_INITIATECONTROLLERS)GetProcAddress(m_module, "InitiateControllers");
                    if (old_initiate_controllers)
                        old_initiate_controllers(g_main_hwnd, g_core.controls);
                }
            }

            run_config();

            break;
        }
    case plugin_rsp:
        {
            if (!core_vr_get_launched())
            {
                auto initiateRSP = (INITIATERSP)GetProcAddress(m_module, "InitiateRSP");
                uint32_t i = 0;
                if (initiateRSP)
                    initiateRSP(dummy_rsp_info, &i);
            }

            run_config();

            break;
        }
    default:
        assert(false);
        break;
    }
}

void Plugin::test()
{
    dll_test = (DLLTEST)GetProcAddress(m_module, "DllTest");
    if (dll_test)
        dll_test(g_hwnd_plug);
}

void Plugin::about()
{
    dll_about = (DLLABOUT)GetProcAddress(m_module, "DllAbout");
    if (dll_about)
        dll_about(g_hwnd_plug);
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

void setup_dummy_info()
{
    int32_t i;

    /////// GFX ///////////////////////////

    dummy_gfx_info.byteswapped = 1;
    dummy_gfx_info.rom = (uint8_t*)dummy_header;
    dummy_gfx_info.rdram = (uint8_t*)g_core.rdram;
    dummy_gfx_info.dmem = (uint8_t*)g_core.SP_DMEM;
    dummy_gfx_info.imem = (uint8_t*)g_core.SP_IMEM;
    dummy_gfx_info.mi_intr_reg = &g_core.MI_register->mi_intr_reg;
    dummy_gfx_info.dpc_start_reg = &g_core.dpc_register->dpc_start;
    dummy_gfx_info.dpc_end_reg = &g_core.dpc_register->dpc_end;
    dummy_gfx_info.dpc_current_reg = &g_core.dpc_register->dpc_current;
    dummy_gfx_info.dpc_status_reg = &g_core.dpc_register->dpc_status;
    dummy_gfx_info.dpc_clock_reg = &g_core.dpc_register->dpc_clock;
    dummy_gfx_info.dpc_bufbusy_reg = &g_core.dpc_register->dpc_bufbusy;
    dummy_gfx_info.dpc_pipebusy_reg = &g_core.dpc_register->dpc_pipebusy;
    dummy_gfx_info.dpc_tmem_reg = &g_core.dpc_register->dpc_tmem;
    dummy_gfx_info.vi_status_reg = &g_core.vi_register->vi_status;
    dummy_gfx_info.vi_origin_reg = &g_core.vi_register->vi_origin;
    dummy_gfx_info.vi_width_reg = &g_core.vi_register->vi_width;
    dummy_gfx_info.vi_intr_reg = &g_core.vi_register->vi_v_intr;
    dummy_gfx_info.vi_v_current_line_reg = &g_core.vi_register->vi_current;
    dummy_gfx_info.vi_timing_reg = &g_core.vi_register->vi_burst;
    dummy_gfx_info.vi_v_sync_reg = &g_core.vi_register->vi_v_sync;
    dummy_gfx_info.vi_h_sync_reg = &g_core.vi_register->vi_h_sync;
    dummy_gfx_info.vi_leap_reg = &g_core.vi_register->vi_leap;
    dummy_gfx_info.vi_h_start_reg = &g_core.vi_register->vi_h_start;
    dummy_gfx_info.vi_v_start_reg = &g_core.vi_register->vi_v_start;
    dummy_gfx_info.vi_v_burst_reg = &g_core.vi_register->vi_v_burst;
    dummy_gfx_info.vi_x_scale_reg = &g_core.vi_register->vi_x_scale;
    dummy_gfx_info.vi_y_scale_reg = &g_core.vi_register->vi_y_scale;
    dummy_gfx_info.check_interrupts = dummy_void;

    /////// AUDIO /////////////////////////
    dummy_audio_info.main_hwnd = g_main_hwnd;
    dummy_audio_info.hinst = g_app_instance;
    dummy_audio_info.byteswapped = 1;
    dummy_audio_info.rom = (uint8_t*)dummy_header;
    dummy_audio_info.rdram = (uint8_t*)g_core.rdram;
    dummy_audio_info.dmem = (uint8_t*)g_core.SP_DMEM;
    dummy_audio_info.imem = (uint8_t*)g_core.SP_IMEM;
    dummy_audio_info.mi_intr_reg = &g_core.MI_register->mi_intr_reg;
    dummy_audio_info.ai_dram_addr_reg = &g_core.ai_register->ai_dram_addr;
    dummy_audio_info.ai_len_reg = &g_core.ai_register->ai_len;
    dummy_audio_info.ai_control_reg = &g_core.ai_register->ai_control;
    dummy_audio_info.ai_status_reg = &g_core.ai_register->ai_status;
    dummy_audio_info.ai_dacrate_reg = &g_core.ai_register->ai_dacrate;
    dummy_audio_info.ai_bitrate_reg = &g_core.ai_register->ai_bitrate;
    dummy_audio_info.check_interrupts = dummy_void;

    ///// CONTROLS ///////////////////////////
    dummy_control_info.main_hwnd = g_main_hwnd;
    dummy_control_info.hinst = g_app_instance;
    dummy_control_info.byteswapped = 1;
    dummy_control_info.header = (uint8_t*)dummy_header;
    dummy_control_info.controllers = g_core.controls;
    for (i = 0; i < 4; i++)
    {
        g_core.controls[i].Present = 0;
        g_core.controls[i].RawData = 0;
        g_core.controls[i].Plugin = (int32_t)ce_none;
    }

    //////// RSP /////////////////////////////
    dummy_rsp_info.byteswapped = 1;
    dummy_rsp_info.rdram = (uint8_t*)g_core.rdram;
    dummy_rsp_info.dmem = (uint8_t*)g_core.SP_DMEM;
    dummy_rsp_info.imem = (uint8_t*)g_core.SP_IMEM;
    dummy_rsp_info.mi_intr_reg = &g_core.MI_register->mi_intr_reg;
    dummy_rsp_info.sp_mem_addr_reg = &g_core.sp_register->sp_mem_addr_reg;
    dummy_rsp_info.sp_dram_addr_reg = &g_core.sp_register->sp_dram_addr_reg;
    dummy_rsp_info.sp_rd_len_reg = &g_core.sp_register->sp_rd_len_reg;
    dummy_rsp_info.sp_wr_len_reg = &g_core.sp_register->sp_wr_len_reg;
    dummy_rsp_info.sp_status_reg = &g_core.sp_register->sp_status_reg;
    dummy_rsp_info.sp_dma_full_reg = &g_core.sp_register->sp_dma_full_reg;
    dummy_rsp_info.sp_dma_busy_reg = &g_core.sp_register->sp_dma_busy_reg;
    dummy_rsp_info.sp_pc_reg = &g_core.rsp_register->rsp_pc;
    dummy_rsp_info.sp_semaphore_reg = &g_core.sp_register->sp_semaphore_reg;
    dummy_rsp_info.dpc_start_reg = &g_core.dpc_register->dpc_start;
    dummy_rsp_info.dpc_end_reg = &g_core.dpc_register->dpc_end;
    dummy_rsp_info.dpc_current_reg = &g_core.dpc_register->dpc_current;
    dummy_rsp_info.dpc_status_reg = &g_core.dpc_register->dpc_status;
    dummy_rsp_info.dpc_clock_reg = &g_core.dpc_register->dpc_clock;
    dummy_rsp_info.dpc_bufbusy_reg = &g_core.dpc_register->dpc_bufbusy;
    dummy_rsp_info.dpc_pipebusy_reg = &g_core.dpc_register->dpc_pipebusy;
    dummy_rsp_info.dpc_tmem_reg = &g_core.dpc_register->dpc_tmem;
    dummy_rsp_info.check_interrupts = dummy_void;
    dummy_rsp_info.process_dlist_list = g_core.plugin_funcs.video_process_dlist;
    dummy_rsp_info.process_alist_list = g_core.plugin_funcs.audio_process_alist;
    dummy_rsp_info.process_rdp_list = g_core.plugin_funcs.video_process_rdp_list;
    dummy_rsp_info.show_cfb = g_core.plugin_funcs.video_show_cfb;
}
