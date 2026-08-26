/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "TASVideo.hpp"
#include "OpenGL.hpp"
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
        RSP_SendMessage(RSPMSG_START);
        return true;
    }

    RSP.halt = FALSE;
    RSP.thread = std::make_unique<std::jthread>(RSP_ThreadProc);
    RSP_SendMessage(RSPMSG_START);
    return true;
}

EXPORT void CALL M64RRGetMetadata(M64RRSpec::PluginMetadata *metadata)
{
    metadata->type = M64RRSpec::PluginType::Video;

    const auto name = PLUGIN_NAME;
    const auto description = "Built-in plugin for Mupen64."
                             "\n\n"
                             "https://mupen64.com";
    const auto target_version = CURRENT_VERSION;

    auto result = std::format_to_n(metadata->name, sizeof(metadata->name) - 1, "{}", name);
    metadata->name[result.size] = '\0';

    result = std::format_to_n(metadata->description, sizeof(metadata->description) - 1, "{}", description);
    metadata->description[result.size] = '\0';

    result = std::format_to_n(metadata->target_version, sizeof(metadata->target_version) - 1, "{}", target_version);
    metadata->target_version[result.size] = '\0';
}

EXPORT void CALL M64RRProcessEvent(Event event)
{
    static int8_t init_count = 0;

    switch (event.type)
    {
    case M64RRSpec::Event::Type::Initiate:
        init_count++;

        g_plugin = event.initiate.init;

        Config_LoadConfig();
        break;
    case M64RRSpec::Event::Type::Shutdown:
        init_count++;
        if (init_count > 0) break;

        if (!RSP.thread) break;
        if (RSP.busy) RSP.halt = TRUE;
        RSP_SendMessage(RSPMSG_CLOSE);
        RSP.thread->join();
        RSP.thread.reset();
        break;
    case M64RRSpec::Event::Type::RomOpened:
        init_rsp_thread();
        OGL_ResizeWindow();
        break;
    case M64RRSpec::Event::Type::RomClosed:
        RSP_SendMessage(RSPMSG_BLACKOUT);
        break;
    default:
        break;
    }
}

EXPORT void CALL M64RRProcessDList(void)
{
    if (RSP.thread)
    {
        RSP_SendMessage(RSPMSG_PROCESSDLIST);
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
            RSP_SendMessage(RSPMSG_READPIXELS);
        }
    }
}

#ifndef _WIN32
EXPORT void CALL M64RRShowConfig(M64RRSpec::WindowHandle)
{
}
#endif
