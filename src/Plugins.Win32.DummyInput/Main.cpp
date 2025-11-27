/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.h"

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
    info->type = plugin_input;
    strncpy_s(info->name, IOUtils::to_utf8_string(PLUGIN_NAME).c_str(), std::size(info->name));
}

EXPORT void CALL DllAbout(void *hParent)
{
    const auto msg = PLUGIN_NAME L"\n"
                                 L"Part of the Mupen64 project family."
                                 L"\n\n"
                                 L"https://github.com/mupen64/mupen64-rr-lua";

    MessageBox((HWND)hParent, msg, L"About", MB_ICONINFORMATION | MB_OK);
}

EXPORT void CALL InitiateControllers(core_input_info ControlInfo)
{
    ControlInfo.controllers[0].Present = true;
}