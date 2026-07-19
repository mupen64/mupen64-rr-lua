/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include <DummyPluginStub.hpp>
#include <VersionNameHelpers.hpp>
#include <core_api.h>
#include <Views.Win32/ZilmarExtSpecPlugin.h>

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME(L"No Input")

DUMMY_PLUGIN_STUB_IMPL(ZilmarExtSpec::PluginType::Input)

EXPORT void CALL InitiateControllers(ZilmarExtSpec::InputPluginInfo ControlInfo)
{
    auto *controllers = ControlInfo.controllers;

    for (int i = 0; i < 4; ++i)
    {
        controllers[i].Present = 0;
        controllers[i].RawData = 0;
        controllers[i].Plugin = CoreControllerExtension::None;
    }

    controllers[0].Present = 1;
}
