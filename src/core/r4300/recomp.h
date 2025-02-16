/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#if defined(__x86_64__)
#include "x86_64/assemble_struct.h"
#else
#include <r4300/x86_64/assemble.h>
#endif

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

typedef struct _precomp_block
{
    precomp_instr *block;
    unsigned int start;
    unsigned int end;
    unsigned char *code;
    unsigned int code_length;
    unsigned int max_code_length;
    void *jumps_table;
    int jumps_number;
    void *riprel_table;
    int riprel_number;
    //unsigned char md5[16];
    unsigned int adler32;
} precomp_block;

void recompile_block(int32_t* source, precomp_block* block, uint32_t func);
void init_block(precomp_block* block);
void free_block(precomp_block *block);
void recompile_opcode();
void prefetch_opcode(uint32_t op);
void dyna_jump();
void dyna_start(void (*code)());
void dyna_stop();

void* malloc_exec(size_t size);
void* realloc_exec(void* ptr, size_t oldsize, size_t newsize);
void free_exec(void* ptr);

extern precomp_instr* dst;