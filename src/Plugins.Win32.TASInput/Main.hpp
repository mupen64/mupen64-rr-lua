/*
 * Copyright (c) 2026, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Windows.h>
#include <m64rr/Plugin.hpp>
#include <WinFilePicker.hpp>
#include <filesystem>

extern HINSTANCE g_inst;
extern M64RRSpec::PluginInit *g_plugin;

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME("TAS Input")

#define NUMBER_OF_CONTROLS 4
