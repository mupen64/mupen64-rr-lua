/**
 * Mupen64 - recomp.h
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

#ifndef RECOMP_H
#define RECOMP_H

#include <cstdint>

#include "x86/assemble.h"

typedef struct _precomp_instr {
    void (*ops)();

    union {
        struct {
            int64_t* rs;
            int64_t* rt;
            int16_t immediate;
        } i;

        struct {
            uint32_t inst_index;
        } j;

        struct {
            int64_t* rs;
            int64_t* rt;
            int64_t* rd;
            unsigned char sa;
            unsigned char nrd;
        } r;

        struct {
            unsigned char base;
            unsigned char ft;
            int16_t offset;
        } lf;

        struct {
            unsigned char ft;
            unsigned char fs;
            unsigned char fd;
        } cf;
#ifdef LUA_BREAKPOINTSYNC_INTERP
        unsigned char stype;
#endif
    } f;

    uint32_t addr;
    uint32_t local_addr;
    reg_cache_struct reg_cache_infos;
    void (*s_ops)();
    uint32_t src;
} precomp_instr;

typedef struct _precomp_block {
    precomp_instr* block;
    uint32_t start;
    uint32_t end;
    unsigned char* code;
    uint32_t code_length;
    uint32_t max_code_length;
    void* jumps_table;
    int32_t jumps_number;
    // unsigned char md5[16];
    uint32_t adler32;
} precomp_block;

void recompile_block(int32_t* source, precomp_block* block, uint32_t func);
void init_block(int32_t* source, precomp_block* block);
void recompile_opcode();
void prefetch_opcode(uint32_t op);
void dyna_jump();
void dyna_start(void (*code)());
void dyna_stop();

/**
 * \brief Recompiles the block at the specified address
 * \param addr The virtual address to invalidate
 */
void recompile_now(uint32_t addr);

/**
 * \brief Invalidates all blocks
 */
void recompile_all();

extern precomp_instr* dst;

#include "x86/regcache.h"

#endif
