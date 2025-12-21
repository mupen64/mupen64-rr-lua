/*
 * Copyright (c) 2025, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Main.h>
#include <TASInput.h>

#define EXPORT __declspec(dllexport)
#define CALL _cdecl

static void log_shim(const wchar_t* str)
{
    wprintf(str);
}

static core_plugin_extended_funcs ef_shim = {
.size = sizeof(core_plugin_extended_funcs),
.log_trace = log_shim,
.log_info = log_shim,
.log_warn = log_shim,
.log_error = log_shim,
};

HINSTANCE g_inst;
core_plugin_extended_funcs* g_ef = &ef_shim;

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

EXPORT void CALL ReceiveExtendedFuncs(core_plugin_extended_funcs* funcs)
{
    g_ef = funcs;
}
