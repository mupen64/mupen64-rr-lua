/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include <R4300/Ops.hpp>
#include <R4300/Recomph.hpp>

// static uint32_t pMFC0 = (uintptr_t)(MFC0);
void genmfc0()
{
    gencallinterp((uintptr_t)MFC0, 0);
    /*dst->local_addr = code_length;
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst));
    call_m32((uint32_t *)(&pMFC0));
    genupdate_system(0);*/
}

// static uint32_t pMTC0 = (uintptr_t)(MTC0);
void genmtc0()
{
    gencallinterp((uintptr_t)MTC0, 0);
    /*dst->local_addr = code_length;
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst));
    call_m32((uint32_t *)(&pMTC0));
    genupdate_system(0);*/
}
