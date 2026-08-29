/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <R4300/R4300.hpp>
#include <R4300/Recomph.hpp>
#include <R4300/x86_64/Assemble.hpp>
#include <R4300/Ops.hpp>

// int64->float converts ported to native SSE (cvtsi2ss / cvtsi2sd, 64-bit source).
// #define INTERPRET_CVT_S_L
// #define INTERPRET_CVT_D_L

void gencvt_s_l()
{
#ifdef INTERPRET_CVT_S_L
    gencallinterp((uintptr_t)CVT_S_L, 0);
#else
    gencheck_cop1_unusable();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    cvtsi2ssq_xmm_preg64(0, EAX); // xmm0 = (float)(int64)fs
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    movss_preg64_xmm(EAX, 0); // fd = xmm0
#endif
}

void gencvt_d_l()
{
#ifdef INTERPRET_CVT_D_L
    gencallinterp((uintptr_t)CVT_D_L, 0);
#else
    gencheck_cop1_unusable();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    cvtsi2sdq_xmm_preg64(0, EAX); // xmm0 = (double)(int64)fs
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    movsd_preg64_xmm(EAX, 0); // fd = xmm0
#endif
}
