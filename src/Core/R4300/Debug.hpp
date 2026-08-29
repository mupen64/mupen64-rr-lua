/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Core/Types.hpp>

void dbg_call_breakpoints_and_wait(const CoreDbgCPUState &state);
CoreBreakpointId dbg_add_breakpoint(uintptr_t address, const CoreBreakpointCallback &callback);
void dbg_remove_breakpoint(const CoreBreakpointId &id);
std::string dbg_disassemble(const CoreDbgCPUState &state);
