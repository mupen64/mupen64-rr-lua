/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main_Win32.hpp"
#include "Config.hpp"
#include "Config_Win32.hpp"
#include "IOUtils.hpp"
#include "Main.hpp"
#include <Views.Win32/ViewPlugin.h>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

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
EXPORT void CALL DllAbout(void *hParent)
{
    const auto msg = L"First-party TAS plugin for Mupen64."
                     L"\n"
                     L"TAS plugins are not to be distributed separately from Mupen64 and remain tied "
                     L"to one version of the emulator."
                     L"\n\n"
                     L"https://mupen64.com";
    MessageBox((HWND)hParent, msg, L"About", MB_ICONASTERISK);
}

EXPORT void CALL DllConfig(void *hParent)
{
    SDLAudio::Config cfg = read_config();
    if (SDLAudio::win32_show_config((HWND)hParent, cfg))
    {
        if (g_ef) g_ef->log_info(L"Saving config...");
        write_config(cfg);
        if (g_backend.has_value()) g_backend->merge_cfg_live(cfg);
    }
}
