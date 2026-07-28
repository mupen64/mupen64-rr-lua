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
    EXPORT void CALL GetDllInfo(ZilmarExtSpec::PluginInfo *info)                                                       \
    {                                                                                                                  \
        info->ver = 0x0101;                                                                                            \
        info->type = plugin_type;                                                                                      \
        strncpy_s(info->name, IOUtils::to_utf8_string(PLUGIN_NAME).c_str(), std::size(info->name));                    \
        std::ranges::copy(IOUtils::to_utf8_string(CURRENT_VERSION), info->target_version);                             \
    }                                                                                                                  \
                                                                                                                       \
    EXPORT void CALL DllAbout(void *hParent)                                                                           \
    {                                                                                                                  \
        const auto msg = L"First-party TAS plugin for Mupen64."                                                        \
                         L"\n"                                                                                         \
                         L"TAS plugins are not to be distributed separately from Mupen64 and remain tied "             \
                         L"to one version of the emulator."                                                            \
                         L"\n\n"                                                                                       \
                         L"https://mupen64.com";                                                                       \
                                                                                                                       \
        MessageBox((HWND)hParent, msg, L"About", MB_ICONINFORMATION | MB_OK);                                          \
    }
