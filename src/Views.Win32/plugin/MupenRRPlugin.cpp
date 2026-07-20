/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <plugin/MupenRRPlugin.hpp>

std::pair<std::wstring, std::unique_ptr<Plugin>> MupenRRPlugin::create(HMODULE module, std::filesystem::path path)
{
    return std::make_pair(L"Unsupported", nullptr);
}

void MupenRRPlugin::config(HWND hwnd)
{
}

void MupenRRPlugin::test(HWND hwnd)
{
}

void MupenRRPlugin::about(HWND hwnd)
{
}

void MupenRRPlugin::initiate()
{
    switch (m_type)
    {
    default:
        RT_ASSERT(false, L"Unsupported plugin type");
        break;
    }
}

void MupenRRPlugin::initiate_dummy()
{
}

void MupenRRPlugin::deinitiate_dummy()
{
}
