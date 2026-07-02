/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include <R4300/Ops.hpp>
#include <R4300/Recomph.hpp>

void gentlbwi()
{
    gencallinterp((uintptr_t)TLBWI, 0);
    /*dst->local_addr = code_length;
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst));
    mov_reg64_imm64(EAX, (uintptr_t)(TLBWI));
    call_reg64(EAX);
    genupdate_system(0);*/
}

void gentlbp()
{
    gencallinterp((uintptr_t)TLBP, 0);
    /*dst->local_addr = code_length;
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst));
    mov_reg64_imm64(EAX, (uintptr_t)(TLBP));
    call_reg64(EAX);
    genupdate_system(0);*/
}

void gentlbr()
{
    gencallinterp((uintptr_t)TLBR, 0);
    /*dst->local_addr = code_length;
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst));
    mov_reg64_imm64(EAX, (uintptr_t)(TLBR));
    call_reg64(EAX);
    genupdate_system(0);*/
}

void generet()
{
    gencallinterp((uintptr_t)ERET, 1);
    /*dst->local_addr = code_length;
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst));
    genupdate_system(0);
    mov_reg64_imm64(EAX, (uintptr_t)(ERET));
    call_reg64(EAX);
    mov_reg64_imm64(EAX, (uintptr_t)(jump_code));
    jmp_reg32(EAX);*/
}

void gentlbwr()
{
    gencallinterp((uintptr_t)TLBWR, 0);
}
