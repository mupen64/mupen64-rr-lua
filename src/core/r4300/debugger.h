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

    void core_compare(void);
}
