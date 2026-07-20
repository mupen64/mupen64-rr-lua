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

std::filesystem::path g_config_path;

HINSTANCE g_inst;
M64RRSpec::ExtendedFuncs *g_ef;

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
