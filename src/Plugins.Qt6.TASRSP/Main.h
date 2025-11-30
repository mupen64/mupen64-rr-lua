/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <CommonPCH.h>
#include <core_api.h>
#include <mupapi.h>

#define PLUGIN_VERSION "1.0.1"

#ifdef _M_X64
#define PLUGIN_ARCH " x64"
#else
#define PLUGIN_ARCH " "
#endif

#ifdef _DEBUG
#define PLUGIN_TARGET " Debug"
#else
#define PLUGIN_TARGET " "
#endif

#define PLUGIN_NAME "TAS RSP Qt6 " PLUGIN_VERSION PLUGIN_ARCH PLUGIN_TARGET

// extern HINSTANCE g_instance;
// extern std::filesystem::path g_app_path;
extern const core_plugin_extended_funcs *g_ef;

bool rsp_alive();
void on_rom_closed();
uint32_t do_rsp_cycles(uint32_t Cycles);
