#include "stdafx.h"
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
    RSP.thread = CreateThread(NULL, 4096, RSP_ThreadProc, NULL, NULL, &thread_id);
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

EXPORT void CALL M64RRLifecycleEvent(LifecycleEvent event)
{
    switch (event.type)
    {
    case M64RRSpec::LifecycleEvent::Type::Initiate:
        g_plugin = event.initiate.init;

        Config_LoadConfig();

        DMEM = g_plugin->dmem;
        IMEM = g_plugin->imem;
        RDRAM = g_plugin->rdram;

        REG.MI_INTR = g_plugin->mi_intr_reg;
        REG.DPC_START = g_plugin->dpc_start_reg;
        REG.DPC_END = g_plugin->dpc_end_reg;
        REG.DPC_CURRENT = g_plugin->dpc_current_reg;
        REG.DPC_STATUS = g_plugin->dpc_status_reg;
        REG.DPC_CLOCK = g_plugin->dpc_clock_reg;
        REG.DPC_BUFBUSY = g_plugin->dpc_bufbusy_reg;
        REG.DPC_PIPEBUSY = g_plugin->dpc_pipebusy_reg;
        REG.DPC_TMEM = g_plugin->dpc_tmem_reg;

        REG.VI_STATUS = g_plugin->vi_status_reg;
        REG.VI_ORIGIN = g_plugin->vi_origin_reg;
        REG.VI_WIDTH = g_plugin->vi_width_reg;
        REG.VI_INTR = g_plugin->vi_intr_reg;
        REG.VI_V_CURRENT_LINE = g_plugin->vi_v_current_line_reg;
        REG.VI_TIMING = g_plugin->vi_timing_reg;
        REG.VI_V_SYNC = g_plugin->vi_v_sync_reg;
        REG.VI_H_SYNC = g_plugin->vi_h_sync_reg;
        REG.VI_LEAP = g_plugin->vi_leap_reg;
        REG.VI_H_START = g_plugin->vi_h_start_reg;
        REG.VI_V_START = g_plugin->vi_v_start_reg;
        REG.VI_V_BURST = g_plugin->vi_v_burst_reg;
        REG.VI_X_SCALE = g_plugin->vi_x_scale_reg;
        REG.VI_Y_SCALE = g_plugin->vi_y_scale_reg;

        init_rsp_thread();
        break;
    case M64RRSpec::LifecycleEvent::Type::RomOpened:
        Config_LoadConfig();
        OGL_ResizeWindow();
        break;
    case M64RRSpec::LifecycleEvent::Type::RomClosed:
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
