/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include <DummyPluginStub.hpp>

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME(L"No Input")

DUMMY_PLUGIN_STUB_IMPL(MupenRRSpecPlugin::PluginType::Input)

EXPORT void CALL M64RRInitiate(MupenRRSpecPlugin::PluginInit *init)
{
    auto *controllers = init->controllers;

    for (int i = 0; i < 4; ++i)
    {
        controllers[i].present = 0;
        controllers[i].raw = 0;
        controllers[i].plugin = MupenRRSpecPlugin::ControllerExtension::None;
    }

    controllers[0].present = 1;
}
