/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Core/stdafx.h>
#include <core_api.h>
#include <core_plugin.h>
#include <resource.h>

#define PLUGIN_VERSION "1.0.0"

#ifdef _M_X64
#define PLUGIN_ARCH " x64"
#else
#define PLUGIN_ARCH " x86"
#endif

#ifdef _DEBUG
#define PLUGIN_TARGET " Debug"
#else
#define PLUGIN_TARGET " Release"
#endif

#define PLUGIN_ISAEXT " SSE2"

#define PLUGIN_NAME "TAS RSP " PLUGIN_VERSION PLUGIN_ARCH PLUGIN_ISAEXT PLUGIN_TARGET

extern HINSTANCE g_instance;
extern std::filesystem::path g_app_path;
extern PlatformService g_platform_service;

bool rsp_alive();
void on_rom_closed();
uint32_t do_rsp_cycles(uint32_t Cycles);
