/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <CommonPCH.hpp>
#include <VersionNameHelpers.hpp>
#include <core_api.h>
#include <Views.Win32/ZilmarExtSpecPlugin.h>

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME(L"TAS RSP")

extern HINSTANCE g_instance;
extern std::filesystem::path g_config_path;
extern core_plugin_extended_funcs *g_ef;

extern void (*ABI1[0x20])();
extern void (*ABI2[0x20])();
extern void (*ABI3[0x20])();

bool rsp_alive();
void on_rom_closed();
uint32_t do_rsp_cycles(uint32_t Cycles);
