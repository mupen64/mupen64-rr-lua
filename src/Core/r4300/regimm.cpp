/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Core.h>
#include <r4300/r4300.h>
#include <r4300/interrupt.h>
#include <r4300/ops.h>
#include <r4300/macros.h>

void BLTZ()
{
    local_rs = core_irs;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (local_rs < 0 && !skip_jump)
        PC += (PC - 2)->f.i.immediate - 1;
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BLTZ_OUT()
{
    local_rs = core_irs;
    jump_target = (int32_t)PC->f.i.immediate;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (!skip_jump && local_rs < 0)
        jump_to(PC->addr + ((jump_target - 1) << 2));
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BLTZ_IDLE()
{
    int32_t skip;
    if (core_irs < 0)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BLTZ();
    }
    else
        BLTZ();
}

void BGEZ()
{
    local_rs = core_irs;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (local_rs >= 0 && !skip_jump)
        PC += (PC - 2)->f.i.immediate - 1;
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BGEZ_OUT()
{
    local_rs = core_irs;
    jump_target = (int32_t)PC->f.i.immediate;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (!skip_jump && local_rs >= 0)
        jump_to(PC->addr + ((jump_target - 1) << 2));
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BGEZ_IDLE()
{
    int32_t skip;
    if (core_irs >= 0)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BGEZ();
    }
    else
        BGEZ();
}

void BLTZL()
{
    if (core_irs < 0)
    {
        PC++;
        delay_slot = 1;
        PC->ops();
        update_count();
        delay_slot = 0;
        if (!skip_jump)
            PC += (PC - 2)->f.i.immediate - 1;
    }
    else
        PC += 2;
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BLTZL_OUT()
{
    if (core_irs < 0)
    {
        jump_target = (int32_t)PC->f.i.immediate;
        PC++;
        delay_slot = 1;
        PC->ops();
        update_count();
        delay_slot = 0;
        if (!skip_jump)
            jump_to(PC->addr + ((jump_target - 1) << 2));
    }
    else
        PC += 2;
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BLTZL_IDLE()
{
    int32_t skip;
    if (core_irs < 0)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BLTZL();
    }
    else
        BLTZL();
}

void BGEZL()
{
    if (core_irs >= 0)
    {
        PC++;
        delay_slot = 1;
        PC->ops();
        update_count();
        delay_slot = 0;
        if (!skip_jump)
            PC += (PC - 2)->f.i.immediate - 1;
    }
    else
        PC += 2;
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BGEZL_OUT()
{
    if (core_irs >= 0)
    {
        jump_target = (int32_t)PC->f.i.immediate;
        PC++;
        delay_slot = 1;
        PC->ops();
        update_count();
        delay_slot = 0;
        if (!skip_jump)
            jump_to(PC->addr + ((jump_target - 1) << 2));
    }
    else
        PC += 2;
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BGEZL_IDLE()
{
    int32_t skip;
    if (core_irs >= 0)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BGEZL();
    }
    else
        BGEZL();
}

void BLTZAL()
{
    local_rs = core_irs;
    reg[31] = PC->addr + 8;
    if ((&core_irs) != (reg + 31))
    {
        PC++;
        delay_slot = 1;
        PC->ops();
        update_count();
        delay_slot = 0;
        if (local_rs < 0 && !skip_jump)
            PC += (PC - 2)->f.i.immediate - 1;
    }
    else
        g_core->log_error(L"erreur dans bltzal");
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BLTZAL_OUT()
{
    local_rs = core_irs;
    reg[31] = PC->addr + 8;
    if ((&core_irs) != (reg + 31))
    {
        jump_target = (int32_t)PC->f.i.immediate;
        PC++;
        delay_slot = 1;
        PC->ops();
        update_count();
        delay_slot = 0;
        if (!skip_jump && local_rs < 0)
            jump_to(PC->addr + ((jump_target - 1) << 2));
    }
    else
        g_core->log_error(L"erreur dans bltzal");
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BLTZAL_IDLE()
{
    int32_t skip;
    if (core_irs < 0)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BLTZAL();
    }
    else
        BLTZAL();
}

void BGEZAL()
{
    local_rs = core_irs;
    reg[31] = PC->addr + 8;
    if ((&core_irs) != (reg + 31))
    {
        PC++;
        delay_slot = 1;
        PC->ops();
        update_count();
        delay_slot = 0;
        if (local_rs >= 0 && !skip_jump)
            PC += (PC - 2)->f.i.immediate - 1;
    }
    else
        g_core->log_info(L"erreur dans bgezal");
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BGEZAL_OUT()
{
    local_rs = core_irs;
    reg[31] = PC->addr + 8;
    if ((&core_irs) != (reg + 31))
    {
        jump_target = (int32_t)PC->f.i.immediate;
        PC++;
        delay_slot = 1;
        PC->ops();
        update_count();
        delay_slot = 0;
        if (!skip_jump && local_rs >= 0)
            jump_to(PC->addr + ((jump_target - 1) << 2));
    }
    else
        g_core->log_info(L"erreur dans bgezal");
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BGEZAL_IDLE()
{
    int32_t skip;
    if (core_irs >= 0)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BGEZAL();
    }
    else
        BGEZAL();
}

void BLTZALL()
{
    local_rs = core_irs;
    reg[31] = PC->addr + 8;
    if ((&core_irs) != (reg + 31))
    {
        if (local_rs < 0)
        {
            PC++;
            delay_slot = 1;
            PC->ops();
            update_count();
            delay_slot = 0;
            if (!skip_jump)
                PC += (PC - 2)->f.i.immediate - 1;
        }
        else
            PC += 2;
    }
    else
        g_core->log_info(L"erreur dans bltzall");
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BLTZALL_OUT()
{
    local_rs = core_irs;
    reg[31] = PC->addr + 8;
    if ((&core_irs) != (reg + 31))
    {
        if (local_rs < 0)
        {
            jump_target = (int32_t)PC->f.i.immediate;
            PC++;
            delay_slot = 1;
            PC->ops();
            update_count();
            delay_slot = 0;
            if (!skip_jump)
                jump_to(PC->addr + ((jump_target - 1) << 2));
        }
        else
            PC += 2;
    }
    else
        g_core->log_info(L"erreur dans bltzall");
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BLTZALL_IDLE()
{
    int32_t skip;
    if (core_irs < 0)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BLTZALL();
    }
    else
        BLTZALL();
}

void BGEZALL()
{
    local_rs = core_irs;
    reg[31] = PC->addr + 8;
    if ((&core_irs) != (reg + 31))
    {
        if (local_rs >= 0)
        {
            PC++;
            delay_slot = 1;
            PC->ops();
            update_count();
            delay_slot = 0;
            if (!skip_jump)
                PC += (PC - 2)->f.i.immediate - 1;
        }
        else
            PC += 2;
    }
    else
        g_core->log_info(L"erreur dans bgezall");
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BGEZALL_OUT()
{
    local_rs = core_irs;
    reg[31] = PC->addr + 8;
    if ((&core_irs) != (reg + 31))
    {
        if (local_rs >= 0)
        {
            jump_target = (int32_t)PC->f.i.immediate;
            PC++;
            delay_slot = 1;
            PC->ops();
            update_count();
            delay_slot = 0;
            if (!skip_jump)
                jump_to(PC->addr + ((jump_target - 1) << 2));
        }
        else
            PC += 2;
    }
    else
        g_core->log_info(L"erreur dans bgezall");
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BGEZALL_IDLE()
{
    int32_t skip;
    if (core_irs >= 0)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BGEZALL();
    }
    else
        BGEZALL();
}
