/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.h>
#include <r4300/Debug.h>
#include <Core.h>

struct Breakpoint
{
    CoreBreakpointId id;
    uintptr_t address;
    CoreBreakpointCallback callback;
};

struct DebuggerState
{
    std::atomic<bool> resumed{true};
    bool advancing{};
    core_dbg_cpu_state cpu_state{};
    std::vector<Breakpoint> breakpoints;
    CoreBreakpointId next_breakpoint_id{0};
};

static DebuggerState s_dbg{};

CoreBreakpointId dbg_add_breakpoint(uintptr_t address, const CoreBreakpointCallback &callback)
{
    CoreBreakpointId id = s_dbg.next_breakpoint_id++;
    s_dbg.breakpoints.push_back({id, address, callback});
    return id;
}

void dbg_remove_breakpoint(const CoreBreakpointId &id)
{
    auto it = std::find_if(s_dbg.breakpoints.begin(), s_dbg.breakpoints.end(),
                           [&](const Breakpoint &bp) { return bp.id == id; });
    if (it != s_dbg.breakpoints.end()) s_dbg.breakpoints.erase(it);
}

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

void dbg_on_late_cycle(const core_dbg_cpu_state &state)
{
    s_dbg.cpu_state = state;

    if (s_dbg.advancing)
    {
        s_dbg.advancing = false;
        s_dbg.resumed = false;

        g_core->callbacks.debugger_resumed_changed(s_dbg.resumed);
    }

    while (!s_dbg.resumed)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
