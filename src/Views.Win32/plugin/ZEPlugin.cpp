/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// ReSharper disable CppCStyleCast

#include "Common.hpp"
#include <Config.hpp>
#include <DialogService.hpp>
#include <components/MGECompositor.hpp>
#include <components/Statusbar.hpp>
#include <plugin/Plugin.hpp>
#include <plugin/ZEPlugin.hpp>

ZESpec::VideoPluginInfo dummy_video_info{};
ZESpec::AudioPluginInfo dummy_audio_info{};
ZESpec::InputPluginInfo dummy_control_info{};
ZESpec::RSPPluginInfo dummy_rsp_info{};
ZESpec::Controller dummy_controllers[4]{};
uint8_t dummy_header[0x40]{};
uint32_t dummy_dw{};

#pragma region Dummy Functions

static uint32_t CALL dummy_do_rsp_cycles(uint32_t Cycles)
{
    return Cycles;
}

static void CALL dummy_void()
{
}

static int32_t CALL dummy_initiate_gfx(ZESpec::VideoPluginInfo)
{
    return 1;
}

static int32_t CALL dummy_initiate_audio(ZESpec::AudioPluginInfo)
{
    return 1;
}

static void CALL dummy_initiate_controllers(ZESpec::InputPluginInfo)
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

static void CALL dummy_get_keys(int32_t, ZESpec::Buttons *)
{
}

static void CALL dummy_set_keys(int32_t, ZESpec::Buttons)
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

static void CALL dummy_initiate_rsp(ZESpec::RSPPluginInfo, uint32_t *)
{
}

static void CALL dummy_fb_read(uint32_t)
{
}

static void CALL dummy_fb_write(uint32_t, uint32_t)
{
}

static void CALL dummy_fb_get_framebuffer_info(ZESpec::FBInfo *)
{
}

static void CALL dummy_move_screen(int32_t, int32_t)
{
}

static void CALL dummy_capture_screen(char *)
{
    if (!PluginUtil::mge_available())
    {
        DialogService::show_dialog(L"The current video plugin doesn't support screenshots.", L"Screenshot", fsvc_error);
        return;
    }

    int32_t width{};
    int32_t height{};
    MGECompositor::get_video_size(&width, &height);

    std::vector<std::uint8_t> video(width * height * 4);
    MGECompositor::copy_video(video.data());

    BITMAPINFOHEADER ihdr;
    ihdr.biSize = sizeof(BITMAPINFOHEADER);
    ihdr.biWidth = width;
    ihdr.biHeight = height;
    ihdr.biPlanes = 1;
    ihdr.biBitCount = 32;
    ihdr.biCompression = BI_RGB;
    ihdr.biSizeImage = width * height * 4;
    ihdr.biXPelsPerMeter = 0;
    ihdr.biYPelsPerMeter = 0;
    ihdr.biClrUsed = 0;
    ihdr.biClrImportant = 0;

    BITMAPFILEHEADER bhdr;
    bhdr.bfType = 19778;
    bhdr.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + ihdr.biSizeImage;
    bhdr.bfReserved1 = bhdr.bfReserved2 = 0;
    bhdr.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    const auto path = Config::screenshot_directory() / std::format("screen{}.bmp", time(nullptr));

    HANDLE hfile;
    hfile = CreateFile(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

    DWORD written;

    WriteFile(hfile, &bhdr, sizeof(BITMAPFILEHEADER), &written, NULL);
    WriteFile(hfile, &ihdr, sizeof(BITMAPINFOHEADER), &written, NULL);
    WriteFile(hfile, video.data(), ihdr.biSizeImage, &written, NULL);

    CloseHandle(hfile);
}

#pragma endregion

std::pair<std::wstring, std::unique_ptr<Plugin>> ZEPlugin::create(HMODULE module, std::filesystem::path path)
{
    const auto get_dll_info = (ZESpec::GETDLLINFO)GetProcAddress(module, "GetDllInfo");

    if (!get_dll_info)
    {
        return std::make_pair(L"GetDllInfo missing", nullptr);
    }

    ZESpec::PluginInfo plugin_info{};
    get_dll_info(&plugin_info);

    const size_t target_version_len = strnlen(plugin_info.target_version, std::size(plugin_info.target_version));
    if (target_version_len > 0)
    {
        // Plugin is tied to one version of mupen
        const auto current_version = IOUtils::to_utf8_string(CURRENT_VERSION);
        const std::string target_version(plugin_info.target_version, target_version_len);
        if (current_version != target_version)
        {
            return std::make_pair(L"Incompatible with this version of Mupen64", nullptr);
        }
    }

    const size_t plugin_name_len = strlen(plugin_info.name);
    while (plugin_info.name[plugin_name_len - 1] == ' ')
    {
        plugin_info.name[plugin_name_len - 1] = '\0';
    }

    auto plugin = std::make_unique<ZEPlugin>();

    plugin->m_path = path;
    plugin->m_name = std::string(plugin_info.name);
    switch (plugin_info.type)
    {
    case ZESpec::PluginType::Video:
        plugin->m_type = Plugin::Type::Video;
        break;
    case ZESpec::PluginType::Audio:
        plugin->m_type = Plugin::Type::Audio;
        break;
    case ZESpec::PluginType::Input:
        plugin->m_type = Plugin::Type::Input;
        break;
    case ZESpec::PluginType::RSP:
        plugin->m_type = Plugin::Type::RSP;
        break;
    default:
        return std::make_pair(L"Unknown plugin type", nullptr);
    }
    plugin->m_version = plugin_info.ver;
    plugin->m_module = module;

    g_view_logger->info("[Plugin] Created plugin {}", plugin->m_name);
    return std::make_pair(L"", std::move(plugin));
}

void ZEPlugin::config(HWND hwnd)
{
    initiate_dummy();

    const auto dll_config = (ZESpec::DLLCONFIG)GetProcAddress(m_module, "DllConfig");

    if (dll_config)
        dll_config(hwnd);
    else
    {
        DialogService::show_dialog(
            std::format(L"'{}' has no configuration.", IOUtils::to_wide_string(this->name())).c_str(), L"Plugin",
            fsvc_error, hwnd);
    }

    deinitiate_dummy();
}

void ZEPlugin::test(HWND hwnd)
{
    initiate_dummy();
    const auto dll_test = (ZESpec::DLLTEST)GetProcAddress(m_module, "DllTest");
    if (dll_test) dll_test(hwnd);
    deinitiate_dummy();
}

void ZEPlugin::about(HWND hwnd)
{
    initiate_dummy();
    const auto dll_about = (ZESpec::DLLABOUT)GetProcAddress(m_module, "DllAbout");
    if (dll_about) dll_about(hwnd);
    deinitiate_dummy();
}

void ZEPlugin::initiate(ZESpecFuncs &funcs)
{
    switch (m_type)
    {
    case Plugin::Type::Video: {
        g_view_logger->trace("Initiating video plugin...");
        ZESpec::INITIATEGFX initiate_gfx{};

        FUNC(funcs.video_change_window, ZESpec::CHANGEWINDOW, dummy_void, "ChangeWindow");
        FUNC(funcs.video_close_dll, ZESpec::CLOSEDLL, dummy_void, "CloseDLL");
        FUNC(initiate_gfx, ZESpec::INITIATEGFX, dummy_initiate_gfx, "InitiateGFX");
        FUNC(funcs.video_process_dlist, ZESpec::PROCESSDLIST, dummy_void, "ProcessDList");
        FUNC(funcs.video_process_rdp_list, ZESpec::PROCESSRDPLIST, dummy_void, "ProcessRDPList");
        FUNC(funcs.video_rom_closed, ZESpec::ROMCLOSED, dummy_void, "RomClosed");
        FUNC(funcs.video_rom_open, ZESpec::ROMOPEN, dummy_void, "RomOpen");
        FUNC(funcs.video_show_cfb, ZESpec::SHOWCFB, dummy_void, "ShowCFB");
        FUNC(funcs.video_update_screen, ZESpec::UPDATESCREEN, dummy_void, "UpdateScreen");
        FUNC(funcs.video_vi_status_changed, ZESpec::VISTATUSCHANGED, dummy_void, "ViStatusChanged");
        FUNC(funcs.video_vi_width_changed, ZESpec::VIWIDTHCHANGED, dummy_void, "ViWidthChanged");
        FUNC(funcs.video_move_screen, ZESpec::MOVESCREEN, dummy_move_screen, "MoveScreen");
        FUNC(funcs.video_capture_screen, ZESpec::CAPTURESCREEN, dummy_capture_screen, "CaptureScreen");
        FUNC(funcs.video_read_screen, ZESpec::READSCREEN, (ZESpec::READSCREEN)GetProcAddress(m_module, "ReadScreen2"),
             "ReadScreen");
        FUNC(funcs.video_get_video_size, ZESpec::GETVIDEOSIZE, nullptr, "mge_get_video_size");
        FUNC(funcs.video_read_video, ZESpec::READVIDEO, nullptr, "mge_read_video2");
        FUNC(funcs.video_fb_read, ZESpec::FBREAD, dummy_fb_read, "FBRead");
        FUNC(funcs.video_fb_write, ZESpec::FBWRITE, dummy_fb_write, "FBWrite");
        FUNC(funcs.video_fb_get_frame_buffer_info, ZESpec::FBGETFRAMEBUFFERINFO, dummy_fb_get_framebuffer_info,
             "FBGetFrameBufferInfo");
        funcs.video_dll_crt_free = PluginUtil::get_free_function_in_module(m_module);

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

        initiate_gfx(gfx_info);

        bool compat_error = false;

        if (!funcs.video_read_video && GetProcAddress(m_module, "mge_read_video")) compat_error = true;

        if (compat_error)
        {
            const auto msg =
                std::format(L"The plugin {} is incompatible with this version of Mupen64 and may not work properly.",
                            IOUtils::to_wide_string(m_name));
            DialogService::show_dialog(msg.c_str(), L"Plugin Incompatibility", fsvc_error);
        }

        break;
    }
    case Plugin::Type::Audio: {
        g_view_logger->trace("Initiating audio plugin...");
        ZESpec::INITIATEAUDIO initiate_audio{};

        FUNC(funcs.audio_close_dll_audio, ZESpec::CLOSEDLL, dummy_void, "CloseDLL");

        ZESpec::AIDACRATECHANGED audio_ai_dacrate_changed{};
        FUNC(audio_ai_dacrate_changed, ZESpec::AIDACRATECHANGED, dummy_ai_dacrate_changed, "AiDacrateChanged");

        funcs.audio_ai_dacrate_changed = [=](CoreSystemType system_type) {
            int32_t ze_system_type{};
            switch (system_type)
            {
            case CoreSystemType::NTSC:
                ze_system_type = 0;
                break;
            case CoreSystemType::PAL:
                ze_system_type = 1;
                break;
            }
            if (audio_ai_dacrate_changed) audio_ai_dacrate_changed(ze_system_type);
        };

        FUNC(funcs.audio_ai_len_changed, ZESpec::AILENCHANGED, dummy_void, "AiLenChanged");
        FUNC(funcs.audio_ai_read_length, ZESpec::AIREADLENGTH, dummy_ai_read_length, "AiReadLength");
        FUNC(initiate_audio, ZESpec::INITIATEAUDIO, dummy_initiate_audio, "InitiateAudio");
        FUNC(funcs.audio_rom_closed, ZESpec::ROMCLOSED, dummy_void, "RomClosed");
        FUNC(funcs.audio_rom_open, ZESpec::ROMOPEN, dummy_void, "RomOpen");
        FUNC(funcs.audio_process_alist, ZESpec::PROCESSALIST, dummy_void, "ProcessAList");
        FUNC(funcs.audio_ai_update, ZESpec::AIUPDATE, nullptr, "AiUpdate");

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

        initiate_audio(audio_info);
        break;
    }
    case Plugin::Type::Input: {
        g_view_logger->trace("Initiating input plugin...");

        ZESpec::OLD_INITIATECONTROLLERS old_initiate_controllers{};
        ZESpec::INITIATECONTROLLERS initiate_controllers{};

        FUNC(funcs.input_close_dll, ZESpec::CLOSEDLL, dummy_void, "CloseDLL");
        FUNC(funcs.input_controller_command, ZESpec::CONTROLLERCOMMAND, dummy_controller_command, "ControllerCommand");
        FUNC(funcs.input_get_keys, ZESpec::GETKEYS, dummy_get_keys, "GetKeys");
        FUNC(funcs.input_set_keys, ZESpec::SETKEYS, dummy_set_keys, "SetKeys");
        if (m_version == 0x0101)
        {
            FUNC(initiate_controllers, ZESpec::INITIATECONTROLLERS, dummy_initiate_controllers, "InitiateControllers");
        }
        else
        {
            FUNC(old_initiate_controllers, ZESpec::OLD_INITIATECONTROLLERS, nullptr, "InitiateControllers");
        }
        FUNC(funcs.input_read_controller, ZESpec::READCONTROLLER, dummy_read_controller, "ReadController");
        FUNC(funcs.input_rom_closed, ZESpec::ROMCLOSED, dummy_void, "RomClosed");
        FUNC(funcs.input_rom_open, ZESpec::ROMOPEN, dummy_void, "RomOpen");
        FUNC(funcs.input_key_down, ZESpec::KEYDOWN, dummy_key_down, "WM_KeyDown");
        FUNC(funcs.input_key_up, ZESpec::KEYUP, dummy_key_up, "WM_KeyUp");

        control_info.main_hwnd = g_main_ctx.hwnd;
        control_info.hinst = g_main_ctx.hinst;
        control_info.byteswapped = 1;
        control_info.header = g_main_ctx.core_ctx->rom;

        std::array<ZESpec::Controller, 4> tmp_controllers{};
        control_info.controllers = tmp_controllers.data();

        if (m_version == 0x0101)
            initiate_controllers(control_info);
        else
            old_initiate_controllers(g_main_ctx.hwnd, tmp_controllers.data());

        for (size_t i = 0; i < std::size(tmp_controllers); ++i)
        {
            g_main_ctx.core.controls[i] = tmp_controllers[i].to_core_controller();
        }
        break;
    }
    case Plugin::Type::RSP: {
        g_view_logger->trace("Initiating RSP plugin...");
        ZESpec::INITIATERSP initiate_rsp{};

        FUNC(funcs.rsp_close_dll, ZESpec::CLOSEDLL, dummy_void, "CloseDLL");
        FUNC(funcs.rsp_do_rsp_cycles, ZESpec::DORSPCYCLES, dummy_do_rsp_cycles, "DoRspCycles");
        FUNC(initiate_rsp, ZESpec::INITIATERSP, dummy_initiate_rsp, "InitiateRSP");
        FUNC(funcs.rsp_rom_closed, ZESpec::ROMCLOSED, dummy_void, "RomClosed");

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
        rsp_info.process_dlist_list = funcs.video_process_dlist;
        rsp_info.process_alist_list = funcs.audio_process_alist;
        rsp_info.process_rdp_list = funcs.video_process_rdp_list;
        rsp_info.show_cfb = funcs.video_show_cfb;

        int32_t i = 4;
        initiate_rsp(rsp_info, (uint32_t *)&i);
        break;
    }
    }
}

void ZEPlugin::initiate_dummy()
{
    Main::init_sdl();

    switch (m_type)
    {
    case Plugin::Type::Video: {
        if (!g_main_ctx.core_ctx->vr_get_launched())
        {
            // NOTE: Since olden days, dummy render target hwnd was the statusbar.
            dummy_video_info.main_hwnd = Statusbar::hwnd();
            dummy_video_info.statusbar_hwnd = Statusbar::hwnd();

            const auto initiate_gfx = (ZESpec::INITIATEGFX)GetProcAddress(m_module, "InitiateGFX");
            if (initiate_gfx && !initiate_gfx(dummy_video_info))
            {
                DialogService::show_dialog(L"Couldn't initialize video plugin.", L"Core", fsvc_information);
            }
        }

        break;
    }
    case Plugin::Type::Audio: {
        if (!g_main_ctx.core_ctx->vr_get_launched())
        {
            const auto initiate_audio = (ZESpec::INITIATEAUDIO)GetProcAddress(m_module, "InitiateAudio");
            if (initiate_audio && !initiate_audio(dummy_audio_info))
            {
                DialogService::show_dialog(L"Couldn't initialize audio plugin.", L"Core", fsvc_information);
            }
        }

        break;
    }
    case Plugin::Type::Input: {
        if (!g_main_ctx.core_ctx->vr_get_launched())
        {
            if (m_version == 0x0101)
            {
                const auto initiate_controllers =
                    (ZESpec::INITIATECONTROLLERS)GetProcAddress(m_module, "InitiateControllers");
                if (initiate_controllers) initiate_controllers(dummy_control_info);
            }
            else
            {
                const auto old_initiate_controllers =
                    (ZESpec::OLD_INITIATECONTROLLERS)GetProcAddress(m_module, "InitiateControllers");

                std::fill(std::begin(dummy_controllers), std::end(dummy_controllers), ZESpec::Controller{});
                if (old_initiate_controllers) old_initiate_controllers(g_main_ctx.hwnd, dummy_controllers);
            }
        }

        break;
    }
    case Plugin::Type::RSP: {
        if (!g_main_ctx.core_ctx->vr_get_launched())
        {
            auto initiateRSP = (ZESpec::INITIATERSP)GetProcAddress(m_module, "InitiateRSP");
            uint32_t i = 0;
            if (initiateRSP) initiateRSP(dummy_rsp_info, &i);
        }

        break;
    }
    default:
        RT_ASSERT(false, L"Unknown plugin type");
    }
}

void ZEPlugin::deinitiate_dummy()
{
    if (g_main_ctx.core_ctx->vr_get_launched()) return;
    const auto close_dll = (ZESpec::CLOSEDLL)GetProcAddress(m_module, "CloseDLL");
    if (close_dll) close_dll();
}
