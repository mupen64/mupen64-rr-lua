/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Common/DummyPluginStub.hpp>

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME("No Input")

DUMMY_PLUGIN_STUB_IMPL(M64RRSpec::PluginType::Input)

EXPORT void CALL M64RRProcessEvent(Event event)
{
    switch (event.type)
    {
    case M64RRSpec::Event::Type::Initiate: {
        auto *controllers = event.initiate.init->controllers;

        for (int i = 0; i < 4; ++i)
        {
            controllers[i].present = 0;
            controllers[i].raw = 0;
            controllers[i].plugin = CoreControllerExtension::None;
        }

        controllers[0].present = 1;
        break;
    }
    default:
        break;
    }
}
