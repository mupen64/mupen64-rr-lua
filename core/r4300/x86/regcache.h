/**
 * Mupen64 - regcache.h
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 *
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
 **/

#ifndef REGCACHE_H
#define REGCACHE_H

#include "../recomp.h"

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

#endif // REGCACHE_H
