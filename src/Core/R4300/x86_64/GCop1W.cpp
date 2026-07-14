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

// int->float converts ported to native SSE (cvtsi2ss / cvtsi2sd).
// #define INTERPRET_CVT_S_W
// #define INTERPRET_CVT_D_W

void gencvt_s_w()
{
#ifdef INTERPRET_CVT_S_W
    gencallinterp((uintptr_t)CVT_S_W, 0);
#else
    gencheck_cop1_unusable();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    cvtsi2ss_xmm_preg64(0, EAX);     // xmm0 = (float)(int32)fs
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    movss_preg64_xmm(EAX, 0);        // fd = xmm0
#endif
}

void gencvt_d_w()
{
#ifdef INTERPRET_CVT_D_W
    gencallinterp((uintptr_t)CVT_D_W, 0);
#else
    gencheck_cop1_unusable();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    cvtsi2sd_xmm_preg64(0, EAX);     // xmm0 = (double)(int32)fs
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    movsd_preg64_xmm(EAX, 0);        // fd = xmm0
#endif
}
