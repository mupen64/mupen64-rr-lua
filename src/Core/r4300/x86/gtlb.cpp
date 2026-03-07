/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.h>
#include <r4300/ops.h>
#include <r4300/recomph.h>

void gentlbwi()
{
    gencallinterp((uint32_t)TLBWI, 0);
    /*dst->local_addr = code_length;
    mov_m32_imm32((void *)(&PC), (uint32_t)(dst));
    mov_reg32_imm32(EAX, (uint32_t)(TLBWI));
    call_reg32(EAX);
    genupdate_system(0);*/
}

void gentlbp()
{
    gencallinterp((uint32_t)TLBP, 0);
    /*dst->local_addr = code_length;
    mov_m32_imm32((void *)(&PC), (uint32_t)(dst));
    mov_reg32_imm32(EAX, (uint32_t)(TLBP));
    call_reg32(EAX);
    genupdate_system(0);*/
}

void gentlbr()
{
    gencallinterp((uint32_t)TLBR, 0);
    /*dst->local_addr = code_length;
    mov_m32_imm32((void *)(&PC), (uint32_t)(dst));
    mov_reg32_imm32(EAX, (uint32_t)(TLBR));
    call_reg32(EAX);
    genupdate_system(0);*/
}

void generet()
{
    gencallinterp((uint32_t)ERET, 1);
    /*dst->local_addr = code_length;
    mov_m32_imm32((void *)(&PC), (uint32_t)(dst));
    genupdate_system(0);
    mov_reg32_imm32(EAX, (uint32_t)(ERET));
    call_reg32(EAX);
    mov_reg32_imm32(EAX, (uint32_t)(jump_code));
    jmp_reg32(EAX);*/
}

void gentlbwr()
{
    gencallinterp((uint32_t)TLBWR, 0);
}
