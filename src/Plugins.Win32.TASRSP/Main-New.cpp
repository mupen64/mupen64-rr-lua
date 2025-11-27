/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.h>
#include <core_api.h>
// #include <PlatformService.h>
#include <Main.h>

// PlatformService platform_service;

// ReSharper disable once CppInconsistentNaming
BOOL APIENTRY DllMain(HMODULE hmod, const DWORD reason, LPVOID)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    default:
        break;
    }

    return TRUE;
}

EXPORT void CALL GetDllInfo(core_plugin_info *info)
{
    info->ver = 0x0101;
    info->type = plugin_rsp;
    strncpy_s(info->name, PLUGIN_NAME, std::size(info->name));
}

EXPORT void CALL DllAbout(void *hParent)
{
    const auto msg = PLUGIN_NAME L"\n"
                                 L"Part of the Mupen64 project family."
                                 L"\n\n"
                                 L"https://github.com/mupen64/mupen64-rr-lua";

    MessageBox((HWND)hParent, msg, L"About", MB_ICONINFORMATION | MB_OK);
}
