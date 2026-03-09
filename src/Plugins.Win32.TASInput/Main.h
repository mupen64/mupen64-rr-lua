/*
 * Copyright (c) 2026, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

extern HINSTANCE g_inst;
extern core_plugin_extended_funcs *g_ef;

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME(L"TAS Input", L"2.0.4")

#define NUMBER_OF_CONTROLS 4

namespace Main
{
void init_sdl();
void pump_sdl_events();
} // namespace Main