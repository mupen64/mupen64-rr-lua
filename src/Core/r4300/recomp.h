/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <r4300/x86/assemble.h>

typedef struct _precomp_instr {
    void (*ops)();

    union {
        struct
        {
            int64_t* rs;
            int64_t* rt;
            int16_t immediate;
        } i;

        struct
        {
            uint32_t inst_index;
        } j;

        struct
        {
            int64_t* rs;
            int64_t* rt;
            int64_t* rd;
            unsigned char sa;
            unsigned char nrd;
        } r;

        struct
        {
            unsigned char base;
            unsigned char ft;
            int16_t offset;
        } lf;

        struct
        {
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
    uint64_t hash;
} precomp_block;

void recompile_block(int32_t* source, precomp_block* block, uint32_t func);
void init_block(int32_t* source, precomp_block* block);
void recompile_opcode();
void prefetch_opcode(uint32_t op);
void dyna_jump();
void dyna_start(void (*code)());
void dyna_stop();

extern precomp_instr* dst;
