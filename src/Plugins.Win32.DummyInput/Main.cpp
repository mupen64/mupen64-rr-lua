/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.h>
#include <DummyPluginStub.h>
#include <VersionNameHelpers.h>
#include <core_api.h>
#include <Views.Win32/ViewPlugin.h>

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME(L"No Input", L"1.0.1")

DUMMY_PLUGIN_STUB_IMPL(plugin_input)

EXPORT void CALL InitiateControllers(core_input_info ControlInfo)
{
    auto *controllers = ControlInfo.controllers;

    for (int i = 0; i < 4; ++i)
    {
        controllers[i].Present = 0;
        controllers[i].RawData = 0;
        controllers[i].Plugin = ce_none;
    }

    controllers[0].Present = 1;
}
