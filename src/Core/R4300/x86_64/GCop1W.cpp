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

// DEBUG (temporary): interpret int->float converts alongside the rest of the FPU.
#define INTERPRET_CVT_S_W
#define INTERPRET_CVT_D_W

void gencvt_s_w()
{
#ifdef INTERPRET_CVT_S_W
    gencallinterp((uintptr_t)CVT_S_W, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    fild_preg32_dword(EAX);
    mov_eax_memoffs32((void *)(&reg_cop1_simple[dst->f.cf.fd]));
    fstp_preg32_dword(EAX);
#endif
}

void gencvt_d_w()
{
#ifdef INTERPRET_CVT_D_W
    gencallinterp((uintptr_t)CVT_D_W, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    fild_preg32_dword(EAX);
    mov_eax_memoffs32((void *)(&reg_cop1_double[dst->f.cf.fd]));
    fstp_preg32_qword(EAX);
#endif
}
