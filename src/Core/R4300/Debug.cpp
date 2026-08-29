/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Core.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Debug.hpp>
#include <R4300/Disasm.hpp>

struct Breakpoint
{
    CoreBreakpointId id;
    CoreBreakpointCallback callback;
};

struct DebuggerState
{
    std::shared_mutex mtx;
    std::atomic<uint32_t> breakpoint_count{0};
    CoreDbgCPUState cpu_state{};
    std::unordered_map<uintptr_t, std::vector<Breakpoint>> breakpoints;
    CoreBreakpointId next_breakpoint_id{0};
};

static DebuggerState s_dbg{};

void dbg_call_breakpoints_and_wait(const CoreDbgCPUState &state)
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

std::string dbg_disassemble(const CoreDbgCPUState &state)
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
