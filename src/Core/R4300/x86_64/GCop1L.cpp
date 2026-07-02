/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Recomph.hpp>
#include <R4300/x86_64/Assemble.hpp>
#include <R4300/Ops.hpp>

// DEBUG (temporary): interpret int64->float converts alongside the rest of the FPU.
#define INTERPRET_CVT_S_L
#define INTERPRET_CVT_D_L

void gencvt_s_l()
{
#ifdef INTERPRET_CVT_S_L
    gencallinterp((uintptr_t)CVT_S_L, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((void *)(&reg_cop1_double[dst->f.cf.fs]));
    fild_preg32_qword(EAX);
    mov_eax_memoffs32((void *)(&reg_cop1_simple[dst->f.cf.fd]));
    fstp_preg32_dword(EAX);
#endif
}

void gencvt_d_l()
{
#ifdef INTERPRET_CVT_D_L
    gencallinterp((uintptr_t)CVT_D_L, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((void *)(&reg_cop1_double[dst->f.cf.fs]));
    fild_preg32_qword(EAX);
    mov_eax_memoffs32((void *)(&reg_cop1_double[dst->f.cf.fd]));
    fstp_preg32_qword(EAX);
#endif
}
