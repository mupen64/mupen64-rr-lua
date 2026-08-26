/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Core.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Recomp.hpp>
#include <R4300/Recomph.hpp>

// NOTE: dynarec isn't compatible with the game debugger

void dyna_jump()
{
    if (PC->reg_cache_infos.need_map)
        *return_address = (uintptr_t)(PC->reg_cache_infos.jump_wrapper);
    else
        *return_address = (uintptr_t)(actual->code + PC->local_addr);
}

jmp_buf g_jmp_state;

void dyna_start(void (*code)())
{
    core_executing = true;
    g_core->callbacks.core_executing_changed(core_executing);
    g_core->log_info(std::format("core_executing: {}", (bool)core_executing));
    if (setjmp(g_jmp_state) == 0)
    {
        code();
    }
}

void dyna_stop()
{
    longjmp(g_jmp_state, 1);
}
