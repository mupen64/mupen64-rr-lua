/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <r4300/r4300.h>
#include <r4300/recomph.h>
#include <r4300/x86/assemble.h>

void gencvt_s_w()
{
#ifdef INTERPRET_CVT_S_W
    gencallinterp((uint32_t)CVT_S_W, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    fild_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    fstp_preg32_dword(EAX);
#endif
}

void gencvt_d_w()
{
#ifdef INTERPRET_CVT_D_W
    gencallinterp((uint32_t)CVT_D_W, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    fild_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    fstp_preg32_qword(EAX);
#endif
}
