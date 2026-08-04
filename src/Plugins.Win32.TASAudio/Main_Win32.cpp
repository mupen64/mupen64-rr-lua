/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include "Main_Win32.hpp"
#include "Config.hpp"
#include "Config_Win32.hpp"
#include "IOUtils.hpp"
#include "Main.hpp"

#include <windows.h>
#include <winerror.h>
#include <winnt.h>
#include <winreg.h>

HINSTANCE g_dll_handle = nullptr;

static constexpr wchar_t CFG_SUBKEY[] = L"Software\\N64 Emulation\\DLL\\TAS Audio";
static constexpr wchar_t VALUE_CONFIG[] = L"Config";
static constexpr wchar_t VALUE_VERSION[] = L"Version";
static constexpr DWORD CUR_CONFIG_VERSION = 1;

BOOL __stdcall DllMain(HINSTANCE hmod, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_dll_handle = hmod;

        std::vector<wchar_t> dll_path_buf(MAX_PATH, L'\0');
        DWORD gmfn_rc = GetModuleFileName(hmod, dll_path_buf.data(), dll_path_buf.size());

        // If the buffer isn't long enough, double the buffer size until it fits
        while (gmfn_rc == dll_path_buf.size())
        {
            dll_path_buf.resize(dll_path_buf.size() * 2);
            gmfn_rc = GetModuleFileName(hmod, dll_path_buf.data(), dll_path_buf.size());
        }

        // set the DLL path
        g_dll_path = std::filesystem::path(dll_path_buf.data());
    }
    return TRUE;
}

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
