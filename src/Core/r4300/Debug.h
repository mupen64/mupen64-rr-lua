/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <include/core_types.h>

void dbg_call_breakpoints_and_wait(const core_dbg_cpu_state &state);
CoreBreakpointId dbg_add_breakpoint(uintptr_t address, const CoreBreakpointCallback &callback);
void dbg_remove_breakpoint(const CoreBreakpointId &id);
std::string dbg_disassemble(const core_dbg_cpu_state &state);
