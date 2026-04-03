/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

void dbg_on_late_cycle(uint32_t opcode, uint32_t address);
bool dbg_get_resumed();
void dbg_set_resumed(bool value);
void dbg_step();
