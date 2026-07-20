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

M64RRSpec::ExtendedFuncs *g_ef;

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

EXPORT void CALL M64RRRShowConfig(WindowHandle parent_window)
{
    Config_Show(parent_window.hwnd());
}

EXPORT void CALL M64RRInitiate(M64RRSpec::PluginInit *init)
{
    g_ef = init->ef;
    g_tas_ctx.config_directory = ZESpec::get_config_path(g_ef);

    Config_LoadConfig();

    DMEM = init->dmem;
    IMEM = init->imem;
    RDRAM = init->rdram;

    REG.MI_INTR = init->mi_intr_reg;
    REG.DPC_START = init->dpc_start_reg;
    REG.DPC_END = init->dpc_end_reg;
    REG.DPC_CURRENT = init->dpc_current_reg;
    REG.DPC_STATUS = init->dpc_status_reg;
    REG.DPC_CLOCK = init->dpc_clock_reg;
    REG.DPC_BUFBUSY = init->dpc_bufbusy_reg;
    REG.DPC_PIPEBUSY = init->dpc_pipebusy_reg;
    REG.DPC_TMEM = init->dpc_tmem_reg;

    REG.VI_STATUS = init->vi_status_reg;
    REG.VI_ORIGIN = init->vi_origin_reg;
    REG.VI_WIDTH = init->vi_width_reg;
    REG.VI_INTR = init->vi_intr_reg;
    REG.VI_V_CURRENT_LINE = init->vi_v_current_line_reg;
    REG.VI_TIMING = init->vi_timing_reg;
    REG.VI_V_SYNC = init->vi_v_sync_reg;
    REG.VI_H_SYNC = init->vi_h_sync_reg;
    REG.VI_LEAP = init->vi_leap_reg;
    REG.VI_H_START = init->vi_h_start_reg;
    REG.VI_V_START = init->vi_v_start_reg;
    REG.VI_V_BURST = init->vi_v_burst_reg;
    REG.VI_X_SCALE = init->vi_x_scale_reg;
    REG.VI_Y_SCALE = init->vi_y_scale_reg;

    init_rsp_thread();
}

EXPORT void CALL M64RRProcessDList(void)
{
    if (RSP.thread)
    {
        SetEvent(RSP.threadMsg[RSPMSG_PROCESSDLIST]);
        WaitForSingleObject(RSP.threadFinished, INFINITE);
    }
}

EXPORT void CALL M64RRRomClosed(void)
{
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
}

EXPORT void CALL M64RRRomOpened(void)
{
    Config_LoadConfig();
    OGL_ResizeWindow();
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
