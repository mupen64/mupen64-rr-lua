/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <CommonPCH.hpp>
#include <VersionNameHelpers.hpp>
#include <m64rr/Plugin.hpp>

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME("TAS RSP")

extern M64RRSpec::PluginInit *g_plugin;

extern void (*ABI1[0x20])();
extern void (*ABI2[0x20])();
extern void (*ABI3[0x20])();

bool rsp_alive();
void on_rom_closed();
uint32_t do_rsp_cycles(uint32_t Cycles);
