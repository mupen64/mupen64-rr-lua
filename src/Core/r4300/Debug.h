/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <include/core_types.h>

CoreBreakpointId dbg_add_breakpoint(uintptr_t address, const CoreBreakpointCallback &callback);
void dbg_remove_breakpoint(const CoreBreakpointId &id);
void dbg_on_late_cycle(const core_dbg_cpu_state &state);
bool dbg_get_resumed();
void dbg_set_resumed(bool value);
void dbg_step();
