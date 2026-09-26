/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <R4300/Recomp.hpp>

#define A64_CACHE_COUNT 8
#define A64_CACHE_FIRST 20

void init_cache(precomp_instr *start);

int32_t allocate_register(uintptr_t addr);

int32_t allocate_register_w(uintptr_t addr);

void free_register(int32_t reg);
void free_all_registers();

void simplify_access();

void build_wrapper(precomp_instr *instr, precomp_block *block);
void build_wrappers(precomp_instr *instr, int32_t start, int32_t end, precomp_block *block);
