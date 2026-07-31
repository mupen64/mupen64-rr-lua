/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "glN64.hpp"
#include "OpenGL.hpp"
#include "N64.hpp"
#include "RSP.hpp"
#include "Config.hpp"

TASVideoContext g_tas_ctx{};

static void log_shim(const wchar_t *str)
{
    wprintf(str);
}

M64RRSpec::PluginInit *g_plugin;

bool init_rsp_thread()
{
    if (RSP.thread)
    {
        SetEvent(RSP.threadMsg[RSPMSG_START]);
        WaitForSingleObject(RSP.threadFinished, INFINITE);
        return true;
    }

    for (auto &i : RSP.threadMsg)
    {
        i = CreateEvent(NULL, FALSE, FALSE, NULL);
        RT_ASSERT(i, L"Error creating video thread message events");
    }

    RSP.threadFinished = CreateEvent(NULL, FALSE, FALSE, NULL);
    RT_ASSERT(RSP.threadFinished, L"Error creating video thread finished event");

    RSP.halt = FALSE;

    DWORD thread_id;
    RSP.thread = CreateThread(NULL, 4096, RSP_ThreadProc, NULL, 0, &thread_id);
    WaitForSingleObject(RSP.threadFinished, INFINITE);

    SetEvent(RSP.threadMsg[RSPMSG_START]);
    WaitForSingleObject(RSP.threadFinished, INFINITE);
    return true;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
    g_tas_ctx.hinst = hinstDLL;
    return TRUE;
}

EXPORT void CALL M64RRGetMetadata(M64RRSpec::PluginMetadata *metadata)
{
    metadata->type = M64RRSpec::PluginType::Video;

    const auto name = IOUtils::to_utf8_string(PLUGIN_NAME);
    const auto description = "First-party TAS plugin for Mupen64."
                             "\n"
                             "TAS plugins are not to be distributed separately from Mupen64 and remain tied "
                             "to one version of the emulator."
                             "\n\n"
                             "https://mupen64.com";
    const auto target_version = IOUtils::to_utf8_string(CURRENT_VERSION);

    auto result = std::format_to_n(metadata->name, sizeof(metadata->name) - 1, "{}", name);
    metadata->name[result.size] = '\0';

    result = std::format_to_n(metadata->description, sizeof(metadata->description) - 1, "{}", description);
    metadata->description[result.size] = '\0';

    result = std::format_to_n(metadata->target_version, sizeof(metadata->target_version) - 1, "{}", target_version);
    metadata->target_version[result.size] = '\0';
}

EXPORT void CALL M64RRProcessEvent(Event event)
{
    switch (event.type)
    {
    case M64RRSpec::Event::Type::Initiate:
        g_plugin = event.initiate.init;

        Config_LoadConfig();

        DMEM = g_plugin->dmem;
        IMEM = g_plugin->imem;
        RDRAM = g_plugin->rdram;

        REG.MI_INTR = &g_plugin->mi_register->mi_intr_reg;
        REG.DPC_START = &g_plugin->dpc_register->dpc_start;
        REG.DPC_END = &g_plugin->dpc_register->dpc_end;
        REG.DPC_CURRENT = &g_plugin->dpc_register->dpc_current;
        REG.DPC_STATUS = &g_plugin->dpc_register->dpc_status;
        REG.DPC_CLOCK = &g_plugin->dpc_register->dpc_clock;
        REG.DPC_BUFBUSY = &g_plugin->dpc_register->dpc_bufbusy;
        REG.DPC_PIPEBUSY = &g_plugin->dpc_register->dpc_pipebusy;
        REG.DPC_TMEM = &g_plugin->dpc_register->dpc_tmem;

        REG.VI_STATUS = &g_plugin->vi_register->vi_status;
        REG.VI_ORIGIN = &g_plugin->vi_register->vi_origin;
        REG.VI_WIDTH = &g_plugin->vi_register->vi_width;
        REG.VI_INTR = &g_plugin->vi_register->vi_v_intr;
        REG.VI_V_CURRENT_LINE = &g_plugin->vi_register->vi_current;
        REG.VI_TIMING = &g_plugin->vi_register->vi_burst;
        REG.VI_V_SYNC = &g_plugin->vi_register->vi_v_sync;
        REG.VI_H_SYNC = &g_plugin->vi_register->vi_h_sync;
        REG.VI_LEAP = &g_plugin->vi_register->vi_leap;
        REG.VI_H_START = &g_plugin->vi_register->vi_h_start;
        REG.VI_V_START = &g_plugin->vi_register->vi_v_start;
        REG.VI_V_BURST = &g_plugin->vi_register->vi_v_burst;
        REG.VI_X_SCALE = &g_plugin->vi_register->vi_x_scale;
        REG.VI_Y_SCALE = &g_plugin->vi_register->vi_y_scale;

        init_rsp_thread();
        break;
    case M64RRSpec::Event::Type::RomOpened:
        Config_LoadConfig();
        OGL_ResizeWindow();
        break;
    case M64RRSpec::Event::Type::RomClosed:
        if (RSP.thread)
        {
            if (RSP.busy)
            {
                RSP.halt = TRUE;
                WaitForSingleObject(RSP.threadFinished, INFINITE);
            }

            SetEvent(RSP.threadMsg[RSPMSG_CLOSE]);
            WaitForSingleObject(RSP.threadFinished, INFINITE);
        }
        break;
    default:
        break;
    }
}

EXPORT void CALL M64RRRShowConfig(WindowHandle parent_window)
{
    Config_Show(parent_window.hwnd());
}

EXPORT void CALL M64RRProcessDList(void)
{
    if (RSP.thread)
    {
        SetEvent(RSP.threadMsg[RSPMSG_PROCESSDLIST]);
        WaitForSingleObject(RSP.threadFinished, INFINITE);
    }
}

EXPORT void CALL M64RRReadVideo(void *buffer, int32_t *width, int32_t *height)
{
    if (width) *width = OGL.width;
    if (height) *height = OGL.height;
    if (buffer)
    {
        extern void *gCapturedPixels;
        gCapturedPixels = buffer;
        if (RSP.thread)
        {
            SetEvent(RSP.threadMsg[RSPMSG_READPIXELS]);
            WaitForSingleObject(RSP.threadFinished, INFINITE);
        }
    }
}
