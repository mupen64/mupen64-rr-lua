/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include <R4300/Macros.hpp>
#include <R4300/Ops.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Recomp.hpp>
#include <R4300/Recomph.hpp>
#include <R4300/x86_64/Assemble.hpp>

// CTC1 ported to native JIT: writes FCR31 + sets the SSE rounding mode (MXCSR), the x64
// analogue of x86's fldcw path. (Was interpreted only because the old #else used x87 fldcw.)
// #define INTERPRET_CTC1

void genmfc1()
{
#ifdef INTERPRET_MFC1
    gencallinterp((uintptr_t)MFC1, 0);
#else
    gencheck_cop1_unusable();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.r.nrd])); // 64-bit: float* pointer
    mov_reg32_preg32(EBX, EAX);
    mov_m32_reg32((uint32_t *)dst->f.r.rt, EBX);
    sar_reg32_imm8(EBX, 31);
    mov_m32_reg32(((uint32_t *)dst->f.r.rt) + 1, EBX);
#endif
}

void gendmfc1()
{
#ifdef INTERPRET_DMFC1
    gencallinterp((uintptr_t)DMFC1, 0);
#else
    gencheck_cop1_unusable();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.r.nrd])); // 64-bit: double* pointer
    mov_reg32_preg32(EBX, EAX);
    mov_reg32_preg32pimm32(ECX, EAX, 4);
    mov_m32_reg32((uint32_t *)dst->f.r.rt, EBX);
    mov_m32_reg32(((uint32_t *)dst->f.r.rt) + 1, ECX);
#endif
}

void gencfc1()
{
#ifdef INTERPRET_CFC1
    gencallinterp((uintptr_t)CFC1, 0);
#else
    gencheck_cop1_unusable();
    if (dst->f.r.nrd == 31)
        mov_eax_memoffs32((uintptr_t *)&FCR31);
    else
        mov_eax_memoffs32((uintptr_t *)&FCR0);
    mov_memoffs32_eax((uint32_t *)dst->f.r.rt);
    sar_reg32_imm8(EAX, 31);
    mov_memoffs32_eax(((uint32_t *)dst->f.r.rt) + 1);
#endif
}

void genmtc1()
{
#ifdef INTERPRET_MTC1
    gencallinterp((uintptr_t)MTC1, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t *)dst->f.r.rt);
    mov_reg64_m64(EBX, (void *)(&reg_cop1_simple[dst->f.r.nrd])); // 64-bit: float* pointer
    mov_preg32_reg32(EBX, EAX);
#endif
}

void gendmtc1()
{
#ifdef INTERPRET_DMTC1
    gencallinterp((uintptr_t)DMTC1, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t *)dst->f.r.rt);
    mov_reg32_m32(EBX, ((uint32_t *)dst->f.r.rt) + 1);
    mov_reg64_m64(EDX, (void *)(&reg_cop1_double[dst->f.r.nrd])); // 64-bit: double* pointer
    mov_preg32_reg32(EDX, EAX);
    mov_preg32pimm32_reg32(EDX, 4, EBX);
#endif
}

void genctc1()
{
#ifdef INTERPRET_CTC1
    gencallinterp((uintptr_t)CTC1, 0);
#else
    gencheck_cop1_unusable();

    if (dst->f.r.nrd != 31) return;
    // FCR31 = (int32)reg[rt]  (rrt32 = low 32 bits of the source register)
    mov_reg64_m64(EAX, (void *)dst->f.r.rt); // RAX = reg[rt]; EAX = low 32 = rrt32
    mov_m32_reg32((void *)&FCR31, EAX);      // FCR31 = EAX
    and_eax_imm32(3);                        // EAX = FCR31 & 3 = N64 rounding mode

    // Keep the interpreter's rounding_mode global in sync (interpreter/savestate paths).
    cmp_eax_imm32(0);
    jne_rj(0);
    int32_t jne0 = code_length - 1;
    mov_m32_imm32((void *)&rounding_mode, (uintptr_t)MUP_ROUND_NEAREST);
    jmp_imm_short(0);
    int32_t done0 = code_length - 1;
    rj_patch(jne0);

    cmp_eax_imm32(1);
    jne_rj(0);
    int32_t jne1 = code_length - 1;
    mov_m32_imm32((void *)&rounding_mode, (uintptr_t)MUP_ROUND_TRUNC);
    jmp_imm_short(0);
    int32_t done1 = code_length - 1;
    rj_patch(jne1);

    cmp_eax_imm32(2);
    jne_rj(0);
    int32_t jne2 = code_length - 1;
    mov_m32_imm32((void *)&rounding_mode, (uintptr_t)MUP_ROUND_CEIL);
    jmp_imm_short(0);
    int32_t done2 = code_length - 1;
    rj_patch(jne2);

    mov_m32_imm32((void *)&rounding_mode, (uintptr_t)MUP_ROUND_FLOOR);

    rj_patch(done0);
    rj_patch(done1);
    rj_patch(done2);

    // Set MXCSR rounding (bits 13-14) from the N64 mode in EAX (analogue of x86's fldcw):
    // N64 {0=near,1=zero,2=+inf,3=-inf} -> MXCSR RC {0=near,3=zero,2=+inf,1=-inf} = (4-m)&3.
    mov_reg32_imm32(EBX, 4);
    sub_reg32_reg32(EBX, EAX);                 // ebx = 4 - mode
    and_reg32_imm32(EBX, 3);                   // ebx = (4 - mode) & 3 = MXCSR RC value
    shl_reg32_imm8(EBX, 13);                   // ebx = RC bits in place (bits 13-14)
    stmxcsr_m32(&g_scratch_mxcsr);
    and_m32_imm32(&g_scratch_mxcsr, ~0x6000u); // clear the RC field
    or_m32_reg32(&g_scratch_mxcsr, EBX);       // set the new RC
    ldmxcsr_m32(&g_scratch_mxcsr);
#endif
}
