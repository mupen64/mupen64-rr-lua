/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <R4300/Ops.hpp>
#include <R4300/Recomph.hpp>

// static uint32_t pMFC0 = (uint32_t)(MFC0);
void genmfc0()
{
    gencallinterp((uint32_t)MFC0, 0);
    /*dst->local_addr = code_length;
    mov_m32_imm32((void *)(&PC), (uint32_t)(dst));
    call_m32((uint32_t *)(&pMFC0));
    genupdate_system(0);*/
}

// static uint32_t pMTC0 = (uint32_t)(MTC0);
void genmtc0()
{
    gencallinterp((uint32_t)MTC0, 0);
    /*dst->local_addr = code_length;
    mov_m32_imm32((void *)(&PC), (uint32_t)(dst));
    call_m32((uint32_t *)(&pMTC0));
    genupdate_system(0);*/
}
