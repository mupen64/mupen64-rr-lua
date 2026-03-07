/*
 * Copyright (c) 2026, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.h"
#include <Main.h>
#include <TASInput.h>
#include <GamepadManager.h>
#include <ConfigDialog.h>

#define EXPORT __declspec(dllexport)
#define CALL _cdecl

static void log_shim(const wchar_t *str)
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

EXPORT void CALL ReceiveExtendedFuncs(core_plugin_extended_funcs *funcs)
{
    g_ef = funcs;
}

void Main::init_sdl()
{
    RT_ASSERT(SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK), L"Failed to initialize SDL subsystems");
}

void Main::pump_sdl_events()
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        GamepadManager::on_sdl_event(e);
        ConfigDialog::on_sdl_event(e);
    }
}