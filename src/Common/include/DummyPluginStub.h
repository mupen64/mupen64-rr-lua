/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#define DUMMY_PLUGIN_STUB_IMPL(plugin_type)                                                                            \
                                                                                                                       \
    BOOL APIENTRY DllMain(HMODULE hmod, const DWORD reason, LPVOID)                                                    \
    {                                                                                                                  \
        return TRUE;                                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    EXPORT void CALL GetDllInfo(core_plugin_info *info)                                                                \
    {                                                                                                                  \
        info->ver = 0x0101;                                                                                            \
        info->type = plugin_type;                                                                                      \
        strncpy_s(info->name, IOUtils::to_utf8_string(PLUGIN_NAME).c_str(), std::size(info->name));                    \
    }                                                                                                                  \
                                                                                                                       \
    EXPORT void CALL DllAbout(void *hParent)                                                                           \
    {                                                                                                                  \
        const auto msg = PLUGIN_NAME L"\n"                                                                             \
                                     L"Part of the Mupen64 project family."                                            \
                                     L"\n\n"                                                                           \
                                     L"https://github.com/mupen64/mupen64-rr-lua";                                     \
                                                                                                                       \
        MessageBox((HWND)hParent, msg, L"About", MB_ICONINFORMATION | MB_OK);                                          \
    }
