/*
 * Copyright (c) 2026, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Main.hpp>
#include <TASInput.hpp>
#include <GamepadManager.hpp>
#include <ConfigDialog.hpp>

#define EXPORT __declspec(dllexport)
#undef CALL
#define CALL _cdecl

std::filesystem::path g_config_path;

static void log_shim(const wchar_t *str)
{
    wprintf(str);
}

static core_plugin_extended_funcs ef_shim{};

HINSTANCE g_inst;
core_plugin_extended_funcs *g_ef = &ef_shim;

// ReSharper disable once CppInconsistentNaming
int WINAPI DllMain(const HINSTANCE h_instance, const DWORD fdw_reason, PVOID)
{
    switch (fdw_reason)
    {
    case DLL_PROCESS_ATTACH:
        g_inst = h_instance;
        break;

    case DLL_PROCESS_DETACH:
        TASInput::on_detach();
        break;
    }

    return TRUE;
}
