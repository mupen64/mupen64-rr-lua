/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.h>
#include <r4300/Debug.h>
#include <Core.h>

struct DebuggerState
{
    std::atomic<bool> resumed{true};
    bool advancing{};
    core_dbg_cpu_state cpu_state{};
};

static DebuggerState s_dbg{};

bool dbg_get_resumed()
{
    return s_dbg.resumed;
}

void dbg_set_resumed(bool value)
{
    if (value) s_dbg.advancing = false;
    s_dbg.resumed = value;
    g_core->callbacks.debugger_resumed_changed(s_dbg.resumed);
}

void dbg_step()
{
    s_dbg.advancing = true;
    s_dbg.resumed = true;
}

void dbg_on_late_cycle(uint32_t opcode, uint32_t address)
{
    s_dbg.cpu_state = {
        .opcode = opcode,
        .address = address,
    };

    if (s_dbg.advancing)
    {
        s_dbg.advancing = false;
        s_dbg.resumed = false;
        g_core->callbacks.debugger_cpu_state_changed(&s_dbg.cpu_state);
        g_core->callbacks.debugger_resumed_changed(s_dbg.resumed);
    }

    while (!s_dbg.resumed)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
