/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Core.h>
#include <r4300/r4300.h>
#include <r4300/recomp.h>
#include <r4300/recomph.h>

// NOTE: dynarec isn't compatible with the game debugger

void dyna_jump()
{
    if (PC->reg_cache_infos.need_map)
        *return_address = (uint32_t)(PC->reg_cache_infos.jump_wrapper);
    else
        *return_address = (uint32_t)(actual->code + PC->local_addr);
}

jmp_buf g_jmp_state;

void dyna_start(void (*code)())
{
    core_executing = true;
    g_core->callbacks.core_executing_changed(core_executing);
    g_core->log_info(std::format(L"core_executing: {}", (bool)core_executing));
    if (setjmp(g_jmp_state) == 0)
    {
        code();
    }
}

void dyna_stop()
{
    longjmp(g_jmp_state, 1);
}
