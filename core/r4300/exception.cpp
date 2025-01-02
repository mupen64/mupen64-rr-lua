/**
 * Mupen64 - exception.c
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 *
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
 **/

#include <shared/services/LoggingService.h>

#include "../memory/memory.h"
#include "exception.h"
#include "macros.h"
#include "r4300.h"
#include "recomph.h"

extern uint32_t interp_addr;

// Unused, this seems to be handled in pure_interp.c prefetch()
void address_error_exception()
{
    g_core_logger->error("address_error_exception");
    stop = 1;
}

// Unused, an TLB entry is marked as invalid
void TLB_invalid_exception()
{
    if (delay_slot)
    {
        skip_jump = 1;
        g_core_logger->error("delay slot\nTLB refill exception");
        stop = 1;
    }
    g_core_logger->error("TLB invalid exception");
    stop = 1;
}

// Unused, 64-bit miss (is this even used on n64?)
void XTLB_refill_exception(uint64_t addresse)
{
    g_core_logger->error("XTLB refill exception");
    stop = 1;
}

// Means no such virtual->physical translation exists
void TLB_refill_exception(uint32_t address, int32_t w)
{
    int32_t usual_handler = 0, i;
    // g_core_logger->error("TLB_refill_exception:{:#06x}\n", address);
    if (!dynacore && w != 2)
        update_count();
    if (w == 1)
        core_Cause = (3 << 2);
    else
        core_Cause = (2 << 2);
    core_BadVAddr = address;
    core_Context = (core_Context & 0xFF80000F) | ((address >> 9) & 0x007FFFF0);
    core_EntryHi = address & 0xFFFFE000;
    if (core_Status & 0x2) // Test de EXL
    {
        if (interpcore)
            interp_addr = 0x80000180;
        else
            jump_to(0x80000180);
        if (delay_slot == 1 || delay_slot == 3)
            core_Cause |= 0x80000000;
        else
            core_Cause &= 0x7FFFFFFF;
    }
    else
    {
        if (!interpcore)
        {
            if (w != 2)
                core_EPC = PC->addr;
            else
                core_EPC = address;
        }
        else
            core_EPC = interp_addr;

        core_Cause &= ~0x80000000;
        core_Status |= 0x2; // EXL=1

        if (address >= 0x80000000 && address < 0xc0000000)
            usual_handler = 1;
        for (i = 0; i < 32; i++)
        {
            if (/*tlb_e[i].v_even &&*/ address >= tlb_e[i].start_even && address <= tlb_e[i].end_even)
                usual_handler = 1;
            if (/*tlb_e[i].v_odd &&*/ address >= tlb_e[i].start_odd && address <= tlb_e[i].end_odd)
                usual_handler = 1;
        }
        if (usual_handler)
        {
            if (interpcore)
                interp_addr = 0x80000180;
            else
                jump_to(0x80000180);
        }
        else
        {
            if (interpcore)
                interp_addr = 0x80000000;
            else
                jump_to(0x80000000);
        }
    }
    if (delay_slot == 1 || delay_slot == 3)
    {
        core_Cause |= 0x80000000;
        core_EPC -= 4;
    }
    else
    {
        core_Cause &= 0x7FFFFFFF;
    }
    if (w != 2)
        core_EPC -= 4;

    if (interpcore)
        last_addr = interp_addr;
    else
        last_addr = PC->addr;

    if (dynacore)
    {
        dyna_jump();
        if (!dyna_interp)
            delay_slot = 0;
    }

    if (!dynacore || dyna_interp)
    {
        dyna_interp = 0;
        if (delay_slot)
        {
            if (interp_addr)
                skip_jump = interp_addr;
            else
                skip_jump = PC->addr;
            next_interrupt = 0;
        }
    }
}

// Unused, aka TLB modified Exception, entry is not writable
void TLB_mod_exception()
{
    g_core_logger->error("TLB mod exception");
    stop = 1;
}

// Unused
void integer_overflow_exception()
{
    g_core_logger->error("integer overflow exception");
    stop = 1;
}

// Unused, handled somewhere else
void coprocessor_unusable_exception()
{
    g_core_logger->error("coprocessor_unusable_exception");
    stop = 1;
}

// General handler, passes execution to default n64 handler
void exception_general()
{
    update_count();
    // EXL bit, 1 = exception level
    core_Status |= 2;

    // Exception return address
    if (!interpcore)
        core_EPC = PC->addr;
    else
        core_EPC = interp_addr;

    // g_core_logger->error("exception, Cause: {:#06x} EPC: {:#06x} \n", Cause,
    // EPC);

    // Highest bit of Cause tells if exception has been executed in branch delay
    // slot delay_slot seems to always be 0 or 1, why is there reference to 3 ? if
    // delay_slot, arrange the registers as it should be on n64 (emu uses a
    // variable)
    if (delay_slot == 1 || delay_slot == 3)
    {
        core_Cause |= 0x80000000;
        core_EPC -= 4;
    }
    else
    {
        core_Cause &= 0x7FFFFFFF; // make sure its cleared?
    }

    // exception handler is always at 0x80000180, continue there
    if (interpcore)
    {
        interp_addr = 0x80000180;
        last_addr = interp_addr;
    }
    else
    {
        jump_to(0x80000180);
        last_addr = PC->addr;
    }
    if (dynacore)
    {
        dyna_jump();
        if (!dyna_interp)
            delay_slot = 0;
    }
    if (!dynacore || dyna_interp)
    {
        dyna_interp = 0;
        if (delay_slot)
        {
            if (interpcore)
                skip_jump = interp_addr;
            else
                skip_jump = PC->addr;
            next_interrupt = 0;
        }
    }
}
