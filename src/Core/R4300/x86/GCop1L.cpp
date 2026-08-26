/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Common/CommonPCH.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Recomph.hpp>
#include <R4300/x86/Assemble.hpp>

void gencvt_s_l()
{
#ifdef INTERPRET_CVT_S_L
    gencallinterp((uint32_t)CVT_S_L, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t *)(&reg_cop1_double[dst->f.cf.fs]));
    fild_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t *)(&reg_cop1_simple[dst->f.cf.fd]));
    fstp_preg32_dword(EAX);
#endif
}

void gencvt_d_l()
{
#ifdef INTERPRET_CVT_D_L
    gencallinterp((uint32_t)CVT_D_L, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t *)(&reg_cop1_double[dst->f.cf.fs]));
    fild_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t *)(&reg_cop1_double[dst->f.cf.fd]));
    fstp_preg32_qword(EAX);
#endif
}
