/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Core.hpp>
#include <R4300/Exception.hpp>
#include <R4300/Ops.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Recomp.hpp>
#include <R4300/Recomph.hpp>
#include <R4300/arm64/Assemble.hpp>
#include <R4300/arm64/RegCache.hpp>

void gentlbwi()
{
    free_all_registers();

    gencallinterp((uintptr_t)TLBWI, 0);
}

void gentlbp()
{
    free_all_registers();

    gencallinterp((uintptr_t)TLBP, 0);
}

void gentlbr()
{
    free_all_registers();

    gencallinterp((uintptr_t)TLBR, 0);
}

void generet()
{
    free_all_registers();

    gencallinterp((uintptr_t)ERET, 1);
}

void gentlbwr()
{
    free_all_registers();

    gencallinterp((uintptr_t)TLBWR, 0);
}
