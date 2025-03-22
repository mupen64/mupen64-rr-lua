/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "r4300.h"
#include "ops.h"
#include "macros.h"
#include "interrupt.h"

void BC1F()
{
    if (check_cop1_unusable())
        return;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if ((FCR31 & 0x800000) == 0 && !skip_jump)
        PC += (PC - 2)->f.i.immediate - 1;
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BC1F_OUT()
{
    if (check_cop1_unusable())
        return;
    jump_target = (int32_t)PC->f.i.immediate;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (!skip_jump && (FCR31 & 0x800000) == 0)
        jump_to(PC->addr + ((jump_target - 1) << 2));
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BC1F_IDLE()
{
    int32_t skip;
    if ((FCR31 & 0x800000) == 0)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BC1F();
    }
    else
        BC1F();
}

void BC1T()
{
    if (check_cop1_unusable())
        return;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if ((FCR31 & 0x800000) != 0 && !skip_jump)
        PC += (PC - 2)->f.i.immediate - 1;
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BC1T_OUT()
{
    if (check_cop1_unusable())
        return;
    jump_target = (int32_t)PC->f.i.immediate;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (!skip_jump && (FCR31 & 0x800000) != 0)
        jump_to(PC->addr + ((jump_target - 1) << 2));
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BC1T_IDLE()
{
    int32_t skip;
    if ((FCR31 & 0x800000) != 0)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BC1T();
    }
    else
        BC1T();
}

void BC1FL()
{
    if (check_cop1_unusable())
        return;
    if ((FCR31 & 0x800000) == 0)
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

void BC1FL_OUT()
{
    if (check_cop1_unusable())
        return;
    if ((FCR31 & 0x800000) == 0)
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

void BC1FL_IDLE()
{
    int32_t skip;
    if ((FCR31 & 0x800000) == 0)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BC1FL();
    }
    else
        BC1FL();
}

void BC1TL()
{
    if (check_cop1_unusable())
        return;
    if ((FCR31 & 0x800000) != 0)
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

void BC1TL_OUT()
{
    if (check_cop1_unusable())
        return;
    if ((FCR31 & 0x800000) != 0)
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

void BC1TL_IDLE()
{
    int32_t skip;
    if ((FCR31 & 0x800000) != 0)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BC1TL();
    }
    else
        BC1TL();
}
