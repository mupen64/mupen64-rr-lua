/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

namespace Debugger
{
    /**
     * \brief Notifies the debugger of a processor cycle ending
     * \param opcode The processor's opcode
     * \param address The processor's address
     */
    void on_late_cycle(uint32_t opcode, uint32_t address);
} // namespace Debugger

bool dbg_get_resumed();
void dbg_set_is_resumed(bool value);
void dbg_step();
bool dbg_get_dma_read_enabled();
void dbg_set_dma_read_enabled(bool value);
bool dbg_get_rsp_enabled();
void dbg_set_rsp_enabled(bool value);
