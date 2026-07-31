/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Recomph.hpp>
#include <R4300/x86/Assemble.hpp>

void gencvt_s_w()
{
#ifdef INTERPRET_CVT_S_W
    gencallinterp((uint32_t)CVT_S_W, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t *)(&reg_cop1_simple[dst->f.cf.fs]));
    fild_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t *)(&reg_cop1_simple[dst->f.cf.fd]));
    fstp_preg32_dword(EAX);
#endif
}

void gencvt_d_w()
{
#ifdef INTERPRET_CVT_D_W
    gencallinterp((uint32_t)CVT_D_W, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t *)(&reg_cop1_simple[dst->f.cf.fs]));
    fild_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t *)(&reg_cop1_double[dst->f.cf.fd]));
    fstp_preg32_qword(EAX);
#endif
}
