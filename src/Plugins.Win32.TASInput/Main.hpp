/*
 * Copyright (c) 2026, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Windows.h>
#include <Views.Win32/M64RRSpec.h>
#include <WinFilePicker.hpp>
#include <filesystem>

extern HINSTANCE g_inst;
extern M64RRSpec::PluginInit *g_plugin;
extern std::filesystem::path g_config_path;

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME(L"TAS Input")

#define NUMBER_OF_CONTROLS 4
