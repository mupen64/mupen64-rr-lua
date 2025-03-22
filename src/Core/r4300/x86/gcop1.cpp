/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <r4300/macros.h>
#include <r4300/r4300.h>
#include <r4300/recomp.h>
#include <r4300/recomph.h>
#include <r4300/x86/assemble.h>

void genmfc1()
{
#ifdef INTERPRET_MFC1
    gencallinterp((uint32_t)MFC1, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.r.nrd]));
    mov_reg32_preg32(EBX, EAX);
    mov_m32_reg32((uint32_t*)dst->f.r.rt, EBX);
    sar_reg32_imm8(EBX, 31);
    mov_m32_reg32(((uint32_t*)dst->f.r.rt) + 1, EBX);
#endif
}

void gendmfc1()
{
#ifdef INTERPRET_DMFC1
    gencallinterp((uint32_t)DMFC1, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.r.nrd]));
    mov_reg32_preg32(EBX, EAX);
    mov_reg32_preg32pimm32(ECX, EAX, 4);
    mov_m32_reg32((uint32_t*)dst->f.r.rt, EBX);
    mov_m32_reg32(((uint32_t*)dst->f.r.rt) + 1, ECX);
#endif
}

void gencfc1()
{
#ifdef INTERPRET_CFC1
    gencallinterp((uint32_t)CFC1, 0);
#else
    gencheck_cop1_unusable();
    if (dst->f.r.nrd == 31)
        mov_eax_memoffs32((uint32_t*)&FCR31);
    else
        mov_eax_memoffs32((uint32_t*)&FCR0);
    mov_memoffs32_eax((uint32_t*)dst->f.r.rt);
    sar_reg32_imm8(EAX, 31);
    mov_memoffs32_eax(((uint32_t*)dst->f.r.rt) + 1);
#endif
}

void genmtc1()
{
#ifdef INTERPRET_MTC1
    gencallinterp((uint32_t)MTC1, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)dst->f.r.rt);
    mov_reg32_m32(EBX, (uint32_t*)(&reg_cop1_simple[dst->f.r.nrd]));
    mov_preg32_reg32(EBX, EAX);
#endif
}

void gendmtc1()
{
#ifdef INTERPRET_DMTC1
    gencallinterp((uint32_t)DMTC1, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)dst->f.r.rt);
    mov_reg32_m32(EBX, ((uint32_t*)dst->f.r.rt) + 1);
    mov_reg32_m32(EDX, (uint32_t*)(&reg_cop1_double[dst->f.r.nrd]));
    mov_preg32_reg32(EDX, EAX);
    mov_preg32pimm32_reg32(EDX, 4, EBX);
#endif
}

void genctc1()
{
#ifdef INTERPRET_CTC1
    gencallinterp((uint32_t)CTC1, 0);
#else
    gencheck_cop1_unusable();

    if (dst->f.r.nrd != 31)
        return;
    mov_eax_memoffs32((uint32_t*)dst->f.r.rt);
    mov_memoffs32_eax((uint32_t*)&FCR31);
    and_eax_imm32(3);

    cmp_eax_imm32(0);
    jne_rj(12);
    mov_m32_imm32((uint32_t*)&rounding_mode, MUP_ROUND_NEAREST); // 10
    jmp_imm_short(48); // 2

    cmp_eax_imm32(1); // 5
    jne_rj(12); // 2
    mov_m32_imm32((uint32_t*)&rounding_mode, MUP_ROUND_TRUNC); // 10
    jmp_imm_short(29); // 2

    cmp_eax_imm32(2); // 5
    jne_rj(12); // 2
    mov_m32_imm32((uint32_t*)&rounding_mode, MUP_ROUND_CEIL); // 10
    jmp_imm_short(10); // 2

    mov_m32_imm32((uint32_t*)&rounding_mode, MUP_ROUND_FLOOR); // 10

    fldcw_m16((uint16_t*)&rounding_mode);
#endif
}
