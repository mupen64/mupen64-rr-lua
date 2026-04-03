/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.h>
#include <r4300/Debug.h>
#include <r4300/disasm.h>
#include <Core.h>

struct Breakpoint
{
    CoreBreakpointId id;
    CoreBreakpointCallback callback;
};

struct DebuggerState
{
    std::atomic<bool> resumed{true};
    bool advancing{};
    core_dbg_cpu_state cpu_state{};
    std::unordered_map<uintptr_t, std::vector<Breakpoint>> breakpoints;
    CoreBreakpointId next_breakpoint_id{0};
};

static DebuggerState s_dbg{};

void dbg_call_breakpoints_and_wait(const core_dbg_cpu_state &state)
{
    auto it = s_dbg.breakpoints.find(state.address);
    if (it != s_dbg.breakpoints.end())
    {
        auto bps_copy = it->second;
        for (const auto &bp : bps_copy)
        {
            bp.callback(state);
        }
    }

    while (!s_dbg.resumed) std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

CoreBreakpointId dbg_add_breakpoint(uintptr_t address, const CoreBreakpointCallback &callback)
{
    CoreBreakpointId id = s_dbg.next_breakpoint_id++;
    s_dbg.breakpoints[address].push_back({id, callback});
    return id;
}

void dbg_remove_breakpoint(const CoreBreakpointId &id)
{
    for (auto &[address, bps] : s_dbg.breakpoints)
    {
        auto it = std::find_if(bps.begin(), bps.end(), [&](const Breakpoint &bp) { return bp.id == id; });
        if (it != bps.end())
        {
            bps.erase(it);
            break;
        }
    }
}

bool dbg_get_resumed()
{
    return s_dbg.resumed;
}

void dbg_set_resumed(bool value)
{
    if (value) s_dbg.advancing = false;
    s_dbg.resumed = value;
}

void dbg_step()
{
    s_dbg.advancing = true;
    s_dbg.resumed = true;
}

std::string dbg_disassemble(const core_dbg_cpu_state &state)
{
    INSTDECODE decode;
    DecodeInstruction(state.opcode, &decode);

    char buf[120]{};
    char *ptr = buf;
    const char *op = GetOpecodeString(&decode);
    while (*op) *ptr++ = *op++;
    *ptr++ = ' ';

    GetOperandString(ptr, &decode, state.address);

    return std::string(buf);
}
