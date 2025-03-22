/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <r4300/recomp.h>

void init_cache(precomp_instr* start);
void free_all_registers();
void free_register(int32_t reg);
int32_t allocate_register(uint32_t* addr);
int32_t allocate_64_register1(uint32_t* addr);
int32_t allocate_64_register2(uint32_t* addr);
int32_t is64(uint32_t* addr);
void build_wrapper(precomp_instr*, unsigned char*, precomp_block*);
void build_wrappers(precomp_instr*, int32_t, int32_t, precomp_block*);
int32_t lru_register();
int32_t allocate_register_w(uint32_t* addr);
int32_t allocate_64_register1_w(uint32_t* addr);
int32_t allocate_64_register2_w(uint32_t* addr);
void set_register_state(int32_t reg, uint32_t* addr, int32_t dirty);
void set_64_register_state(int32_t reg1, int32_t reg2, uint32_t* addr, int32_t dirty);
void lock_register(int32_t reg);
void unlock_register(int32_t reg);
void allocate_register_manually(int32_t reg, uint32_t* addr);
void allocate_register_manually_w(int32_t reg, uint32_t* addr, int32_t load);
void force_32(int32_t reg);
int32_t lru_register_exc1(int32_t exc1);
void simplify_access();
