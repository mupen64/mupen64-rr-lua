/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Common.Win32/Common.hpp>
#include <Core/Plugin.hpp>
#include <Common.Win32/WinFilePicker.hpp>
#include <filesystem>

extern HINSTANCE g_inst;

namespace BuiltinTAS::TASInput
{
extern M64RRSpec::PluginInit *g_plugin;
}
// technically this is bad practice, but we only need it to isolate g_plugin
using namespace BuiltinTAS::TASInput;

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME("TAS Input")

#define NUMBER_OF_CONTROLS 4
