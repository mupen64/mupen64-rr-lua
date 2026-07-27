/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

// Must match the backend CMake selects (see src/Core/CMakeLists.txt).
#if defined(_M_X64) || defined(__x86_64__)
#include <R4300/x86_64/Assemble.hpp>
#elif defined(_M_IX86) || defined(__i386__)
#include <R4300/x86/Assemble.hpp>
#elif defined(MUPEN64RR_ENABLE_DYNAREC)
#error "No dynarec backend exists for this architecture; build with MUPEN64RR_ENABLE_DYNAREC=OFF."
#else
// Interpreter-only build on a non-x86 host: nothing reads these fields, but
// precomp_instr must still have a member of this type. Recomp.cpp does write
// need_map unconditionally, so the field has to exist.
typedef struct _reg_cache_struct
{
    int32_t need_map;
} reg_cache_struct;
#endif

typedef struct _precomp_instr
{
    void (*ops)();

    union {
        struct
        {
            int64_t *rs;
            int64_t *rt;
            int16_t immediate;
        } i;

        struct
        {
            uint32_t inst_index;
        } j;

        struct
        {
            int64_t *rs;
            int64_t *rt;
            int64_t *rd;
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
    } f;

    uintptr_t addr;
    uintptr_t local_addr;
    reg_cache_struct reg_cache_infos;
    void (*s_ops)();
    uintptr_t src;
} precomp_instr;

typedef struct _precomp_block
{
    precomp_instr *block;
    uintptr_t start;
    uintptr_t end;
    unsigned char *code;
    uint32_t code_length;
    uint32_t max_code_length;
    void *jumps_table;
    int32_t jumps_number;
    uint64_t hash;
} precomp_block;

void recompile_block(int32_t *source, precomp_block *block, uint32_t func);
void init_block(int32_t *source, precomp_block *block);
void recompile_opcode();
void prefetch_opcode(uint32_t op);
void dyna_jump();
void dyna_start(void (*code)());
void dyna_stop();
void vr_recompile(uint32_t addr);

extern precomp_instr *dst;
