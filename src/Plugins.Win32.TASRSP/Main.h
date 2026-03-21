/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <CommonPCH.h>
#include <VersionNameHelpers.h>
#include <core_api.h>
#include <Views.Win32/ViewPlugin.h>
#include <Resource.h>

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME(L"TAS RSP", L"1.0.1")

extern HINSTANCE g_instance;
extern std::filesystem::path g_app_path;
extern core_plugin_extended_funcs *g_ef;

extern void (*ABI1[0x20])();
extern void (*ABI2[0x20])();
extern void (*ABI3[0x20])();

bool rsp_alive();
void on_rom_closed();
uint32_t do_rsp_cycles(uint32_t Cycles);
