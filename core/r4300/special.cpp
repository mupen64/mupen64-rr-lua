/**
 * Mupen64 - special.c
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
#include "interrupt.h"
#include "macros.h"
#include "ops.h"
#include "r4300.h"

void NOP() { PC++; }

void SLL()
{
    rrd32 = (uint32_t)(rrt32) << core_rsa;
    sign_extended(core_rrd);
    PC++;
}

void SRL()
{
    rrd32 = (uint32_t)rrt32 >> core_rsa;
    sign_extended(core_rrd);
    PC++;
}

void SRA()
{
    rrd32 = (int32_t)rrt32 >> core_rsa;
    sign_extended(core_rrd);
    PC++;
}

void SLLV()
{
    rrd32 = (uint32_t)(rrt32) << (rrs32 & 0x1F);
    sign_extended(core_rrd);
    PC++;
}

void SRLV()
{
    rrd32 = (uint32_t)rrt32 >> (rrs32 & 0x1F);
    sign_extended(core_rrd);
    PC++;
}

void SRAV()
{
    rrd32 = (int32_t)rrt32 >> (rrs32 & 0x1F);
    sign_extended(core_rrd);
    PC++;
}

void JR()
{
    local_rs32 = irs32;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    jump_to(local_rs32);
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void JALR()
{
    uint64_t* dest = (uint64_t*)PC->f.r.rd;
    local_rs32 = rrs32;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (!skip_jump)
    {
        *dest = PC->addr;
        sign_extended(*dest);

        jump_to(local_rs32);
    }
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void SYSCALL()
{
    core_Cause = 8 << 2;
    exception_general();
}

void SYNC()
{
#ifdef LUA_BREAKPOINTSYNC_INTERP
    LuaBreakpointSyncInterp();
#else
    PC++;
#endif
}

void MFHI()
{
    core_rrd = hi;
    PC++;
}

void MTHI()
{
    hi = core_rrs;
    PC++;
}

void MFLO()
{
    core_rrd = lo;
    PC++;
}

void MTLO()
{
    lo = core_rrs;
    PC++;
}

void DSLLV()
{
    core_rrd = core_rrt << (rrs32 & 0x3F);
    PC++;
}

void DSRLV()
{
    core_rrd = (uint64_t)core_rrt >> (rrs32 & 0x3F);
    PC++;
}

void DSRAV()
{
    core_rrd = (int64_t)core_rrt >> (rrs32 & 0x3F);
    PC++;
}

void MULT()
{
    int64_t temp;
    temp = core_rrs * core_rrt;
    hi = temp >> 32;
    lo = temp;
    sign_extended(lo);
    PC++;
}

void MULTU()
{
    uint64_t temp;
    temp = (uint32_t)core_rrs * (uint64_t)((uint32_t)core_rrt);
    hi = (int64_t)temp >> 32;
    lo = temp;
    sign_extended(lo);
    PC++;
}

void DIV()
{
    if (rrt32)
    {
        lo = rrs32 / rrt32;
        hi = rrs32 % rrt32;
        sign_extended(lo);
        sign_extended(hi);
    }
    else
        g_core_logger->info("div");
    PC++;
}

void DIVU()
{
    if (rrt32)
    {
        lo = (uint32_t)rrs32 / (uint32_t)rrt32;
        hi = (uint32_t)rrs32 % (uint32_t)rrt32;
        sign_extended(lo);
        sign_extended(hi);
    }
    else
        g_core_logger->info("divu");
    PC++;
}

void DMULT()
{
    uint64_t op1, op2, op3, op4;
    uint64_t result1, result2, result3, result4;
    uint64_t temp1, temp2, temp3, temp4;
    int32_t sign = 0;

    if (core_rrs < 0)
    {
        op2 = -core_rrs;
        sign = 1 - sign;
    }
    else
        op2 = core_rrs;
    if (core_rrt < 0)
    {
        op4 = -core_rrt;
        sign = 1 - sign;
    }
    else
        op4 = core_rrt;

    op1 = op2 & 0xFFFFFFFF;
    op2 = (op2 >> 32) & 0xFFFFFFFF;
    op3 = op4 & 0xFFFFFFFF;
    op4 = (op4 >> 32) & 0xFFFFFFFF;

    temp1 = op1 * op3;
    temp2 = (temp1 >> 32) + op1 * op4;
    temp3 = op2 * op3;
    temp4 = (temp3 >> 32) + op2 * op4;

    result1 = temp1 & 0xFFFFFFFF;
    result2 = temp2 + (temp3 & 0xFFFFFFFF);
    result3 = (result2 >> 32) + temp4;
    result4 = (result3 >> 32);

    lo = result1 | (result2 << 32);
    hi = (result3 & 0xFFFFFFFF) | (result4 << 32);
    if (sign)
    {
        hi = ~hi;
        if (!lo)
            hi++;
        else
            lo = ~lo + 1;
    }
    PC++;
}

void DMULTU()
{
    uint64_t op1, op2, op3, op4;
    uint64_t result1, result2, result3, result4;
    uint64_t temp1, temp2, temp3, temp4;

    op1 = core_rrs & 0xFFFFFFFF;
    op2 = (core_rrs >> 32) & 0xFFFFFFFF;
    op3 = core_rrt & 0xFFFFFFFF;
    op4 = (core_rrt >> 32) & 0xFFFFFFFF;

    temp1 = op1 * op3;
    temp2 = (temp1 >> 32) + op1 * op4;
    temp3 = op2 * op3;
    temp4 = (temp3 >> 32) + op2 * op4;

    result1 = temp1 & 0xFFFFFFFF;
    result2 = temp2 + (temp3 & 0xFFFFFFFF);
    result3 = (result2 >> 32) + temp4;
    result4 = (result3 >> 32);

    lo = result1 | (result2 << 32);
    hi = (result3 & 0xFFFFFFFF) | (result4 << 32);

    PC++;
}

void DDIV()
{
    if (core_rrt)
    {
        lo = (int64_t)core_rrs / (int64_t)core_rrt;
        hi = (int64_t)core_rrs % (int64_t)core_rrt;
    }
    //   else g_core_logger->info("ddiv");
    PC++;
}

void DDIVU()
{
    if (core_rrt)
    {
        lo = (uint64_t)core_rrs / (uint64_t)core_rrt;
        hi = (uint64_t)core_rrs % (uint64_t)core_rrt;
    }
    //   else g_core_logger->info("ddivu");
    PC++;
}

void ADD()
{
    rrd32 = rrs32 + rrt32;
    sign_extended(core_rrd);
    PC++;
}

void ADDU()
{
    rrd32 = rrs32 + rrt32;
    sign_extended(core_rrd);
    PC++;
}

void SUB()
{
    rrd32 = rrs32 - rrt32;
    sign_extended(core_rrd);
    PC++;
}

void SUBU()
{
    rrd32 = rrs32 - rrt32;
    sign_extended(core_rrd);
    PC++;
}

void AND()
{
    core_rrd = core_rrs & core_rrt;
    PC++;
}

void OR()
{
    core_rrd = core_rrs | core_rrt;
    PC++;
}

void XOR()
{
    core_rrd = core_rrs ^ core_rrt;
    PC++;
}

void NOR()
{
    core_rrd = ~(core_rrs | core_rrt);
    PC++;
}

void SLT()
{
    if (core_rrs < core_rrt)
        core_rrd = 1;
    else
        core_rrd = 0;
    PC++;
}

void SLTU()
{
    if ((uint64_t)core_rrs < (uint64_t)core_rrt)
        core_rrd = 1;
    else
        core_rrd = 0;
    PC++;
}

void DADD()
{
    core_rrd = core_rrs + core_rrt;
    PC++;
}

void DADDU()
{
    core_rrd = core_rrs + core_rrt;
    PC++;
}

void DSUB()
{
    core_rrd = core_rrs - core_rrt;
    PC++;
}

void DSUBU()
{
    core_rrd = core_rrs - core_rrt;
    PC++;
}

void TEQ()
{
    if (core_rrs == core_rrt)
    {
        g_core_logger->info("trap exception in teq");
        stop = 1;
    }
    PC++;
}

void DSLL()
{
    core_rrd = core_rrt << core_rsa;
    PC++;
}

void DSRL()
{
    core_rrd = (uint64_t)core_rrt >> core_rsa;
    PC++;
}

void DSRA()
{
    core_rrd = core_rrt >> core_rsa;
    PC++;
}

void DSLL32()
{
    core_rrd = core_rrt << (32 + core_rsa);
    PC++;
}

void DSRL32()
{
    core_rrd = (uint64_t)core_rrt >> (32 + core_rsa);
    PC++;
}

void DSRA32()
{
    core_rrd = (int64_t)core_rrt >> (32 + core_rsa);
    PC++;
}
