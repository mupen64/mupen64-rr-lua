/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <r4300/debugger.h>
#include <Core.h>

bool g_resumed = true;
bool g_instruction_advancing = false;
bool g_dma_read_enabled = true;
core_dbg_cpu_state g_cpu_state{};

DORSPCYCLES g_original_do_rsp_cycles;

static uint32_t dummy_doRspCycles(uint32_t cycles)
{
    return cycles;
}

bool core_dbg_get_resumed()
{
    return g_resumed;
}

void core_dbg_set_is_resumed(bool value)
{
    if (value)
    {
        g_instruction_advancing = false;
    }
    g_resumed = value;
    g_core->callbacks.debugger_resumed_changed(g_resumed);
}

void core_dbg_step()
{
    g_instruction_advancing = true;
    g_resumed = true;
}

bool core_dbg_get_dma_read_enabled()
{
    return g_dma_read_enabled;
}

void core_dbg_set_dma_read_enabled(bool value)
{
    g_dma_read_enabled = value;
}

bool core_dbg_get_rsp_enabled()
{
    return g_core->plugin_funcs.rsp_do_rsp_cycles == g_original_do_rsp_cycles;
}

void core_dbg_set_rsp_enabled(bool value)
{
    // Stash the original plugin-provided do_rsp_cycles once
    if (!g_original_do_rsp_cycles)
    {
        g_original_do_rsp_cycles = g_core->plugin_funcs.rsp_do_rsp_cycles;
    }

    // If RSP is disabled, we swap out the real do_rsp_cycles function for the dummy one thereby effectively disabling the rsp unit
    if (value)
        g_core->plugin_funcs.rsp_do_rsp_cycles = g_original_do_rsp_cycles;
    else
        g_core->plugin_funcs.rsp_do_rsp_cycles = dummy_doRspCycles;
}

void Debugger::on_late_cycle(uint32_t opcode, uint32_t address)
{
    g_cpu_state = {
    .opcode = opcode,
    .address = address,
    };

    if (g_instruction_advancing)
    {
        g_instruction_advancing = false;
        g_resumed = false;
        g_core->callbacks.debugger_cpu_state_changed(&g_cpu_state);
        g_core->callbacks.debugger_resumed_changed(g_resumed);
    }

    while (!g_resumed)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
