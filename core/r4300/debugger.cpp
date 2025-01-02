#include "debugger.h"
#include "Plugin.hpp"

#include <thread>

#include "shared/Messenger.h"

bool g_resumed = true;
bool g_instruction_advancing = false;
bool g_dma_read_enabled = true;
Debugger::t_cpu_state g_cpu_state{};

DORSPCYCLES g_original_do_rsp_cycles;

bool Debugger::get_resumed() { return g_resumed; }

void Debugger::set_is_resumed(bool value)
{
    if (value)
    {
        g_instruction_advancing = false;
    }
    g_resumed = value;
    Messenger::broadcast(Messenger::Message::DebuggerResumedChanged, g_resumed);
}

void Debugger::step()
{
    g_instruction_advancing = true;
    g_resumed = true;
}

bool Debugger::get_dma_read_enabled() { return g_dma_read_enabled; }

void Debugger::set_dma_read_enabled(bool value) { g_dma_read_enabled = value; }

bool Debugger::get_rsp_enabled() { return doRspCycles == g_original_do_rsp_cycles; }

void Debugger::set_rsp_enabled(bool value)
{
    // Stash the original plugin-provided doRspCycles once
    if (!g_original_do_rsp_cycles)
    {
        g_original_do_rsp_cycles = doRspCycles;
    }

    // If RSP is disabled, we swap out the real doRspCycles function for the dummy
    // one thereby effectively disabling the rsp unit
    if (value)
        doRspCycles = g_original_do_rsp_cycles;
    else
        doRspCycles = dummy_doRspCycles;
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
        Messenger::broadcast(Messenger::Message::DebuggerCpuStateChanged, g_cpu_state);
        Messenger::broadcast(Messenger::Message::DebuggerResumedChanged, g_resumed);
    }

    while (!g_resumed)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
