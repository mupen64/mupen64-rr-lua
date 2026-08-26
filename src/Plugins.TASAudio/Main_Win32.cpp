/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifdef _WIN32

#include "Main_Win32.hpp"
#include "Config.hpp"
#include "Config_Win32.hpp"
#include "Main.hpp"
#include <windows.h>
#include <winerror.h>
#include <winnt.h>
#include <winreg.h>

EXPORT void CALL M64RRShowConfig(WindowHandle parent_window)
{
    SDLAudio::Config cfg = read_config();
    if (SDLAudio::win32_show_config(parent_window.hwnd(), cfg))
    {
        if (g_plugin) g_plugin->log_info("Saving config...");
        write_config(cfg);
        if (g_backend.has_value()) g_backend->merge_cfg_live(cfg);
    }
}

#endif
