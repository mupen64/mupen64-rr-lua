/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.h>
#include <Core.h>
#include <r4300/r4300.h>
#include <r4300/Debug.h>
#include <r4300/disasm.h>

struct Breakpoint
{
    CoreBreakpointId id;
    CoreBreakpointCallback callback;
};

struct DebuggerState
{
    std::shared_mutex mtx;
    std::atomic<uint32_t> breakpoint_count{0};
    core_dbg_cpu_state cpu_state{};
    std::unordered_map<uintptr_t, std::vector<Breakpoint>> breakpoints;
    CoreBreakpointId next_breakpoint_id{0};
};

static DebuggerState s_dbg{};

void dbg_call_breakpoints_and_wait(const core_dbg_cpu_state &state)
{
    if (s_dbg.breakpoint_count == 0) return;

    std::shared_lock lock(s_dbg.mtx);
    const auto it = s_dbg.breakpoints.find(state.address);
    if (it == s_dbg.breakpoints.end()) return;

    const auto bps_copy = it->second;
    lock.unlock();

    for (const auto &bp : bps_copy) bp.callback(state);
}

CoreBreakpointId dbg_add_breakpoint(uintptr_t address, const CoreBreakpointCallback &callback)
{
    std::unique_lock lock(s_dbg.mtx);
    CoreBreakpointId id = s_dbg.next_breakpoint_id++;
    s_dbg.breakpoints[address].push_back({id, callback});
    s_dbg.breakpoint_count++;
    return id;
}

void dbg_remove_breakpoint(const CoreBreakpointId &id)
{
    std::unique_lock lock(s_dbg.mtx);
    for (auto &[address, bps] : s_dbg.breakpoints)
    {
        auto it = std::find_if(bps.begin(), bps.end(), [&](const Breakpoint &bp) { return bp.id == id; });
        if (it != bps.end())
        {
            bps.erase(it);
            s_dbg.breakpoint_count--;
            break;
        }
    }
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
