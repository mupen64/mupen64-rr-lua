/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <r4300/macros.h>
#include <r4300/recomph.h>
#include <r4300/x86/assemble.h>
#include <r4300/x86/regcache.h>
#include <alloc.h>

typedef struct _jump_table {
    uint32_t mi_addr;
    uint32_t pc_addr;
} jump_table;

static jump_table* jumps_table = NULL;
static int32_t jumps_number, max_jumps_number;

void init_assembler(void* block_jumps_table, int32_t block_jumps_number)
{
    if (block_jumps_table)
    {
        jumps_table = (jump_table*)block_jumps_table;
        jumps_number = block_jumps_number;
        max_jumps_number = jumps_number;
    }
    else
    {
        jumps_table = (jump_table*)malloc(JUMP_TABLE_SIZE * sizeof(jump_table));
        jumps_number = 0;
        max_jumps_number = JUMP_TABLE_SIZE;
    }
}

void free_assembler(void** block_jumps_table, int32_t* block_jumps_number)
{
    *block_jumps_table = jumps_table;
    *block_jumps_number = jumps_number;
}

static void add_jump(uint32_t pc_addr, uint32_t mi_addr)
{
    if (jumps_number == max_jumps_number)
    {
        max_jumps_number += JUMP_TABLE_SIZE;
        jumps_table = (jump_table*)realloc(jumps_table, max_jumps_number * sizeof(jump_table));
    }
    jumps_table[jumps_number].pc_addr = pc_addr;
    jumps_table[jumps_number].mi_addr = mi_addr;
    jumps_number++;
}

void passe2(precomp_instr* dest, int32_t start, int32_t end, precomp_block* block)
{
    uint32_t i, real_code_length, addr_dest;
    build_wrappers(dest, start, end, block);
    real_code_length = code_length;

    for (i = 0; i < jumps_number; i++)
    {
        code_length = jumps_table[i].pc_addr;
        if (dest[(jumps_table[i].mi_addr - dest[0].addr) / 4].reg_cache_infos.need_map)
        {
            addr_dest = (uint32_t)dest[(jumps_table[i].mi_addr - dest[0].addr) / 4].reg_cache_infos.jump_wrapper;
            put32(addr_dest - ((uint32_t)block->code + code_length) - 4);
        }
        else
        {
            addr_dest = dest[(jumps_table[i].mi_addr - dest[0].addr) / 4].local_addr;
            put32(addr_dest - code_length - 4);
        }
    }
    code_length = real_code_length;
}

inline void put8(unsigned char octet)
{
    (*inst_pointer)[code_length] = octet;
    code_length++;
    if (code_length == max_code_length)
    {
        max_code_length += JUMP_TABLE_SIZE;
        *inst_pointer = (unsigned char*)realloc_exec(*inst_pointer, code_length, max_code_length);
    }
}

inline void put32(uint32_t dword)
{
    if ((code_length + 4) >= max_code_length)
    {
        max_code_length += JUMP_TABLE_SIZE;
        *inst_pointer = (unsigned char*)realloc_exec(*inst_pointer, code_length, max_code_length);
    }
    *((uint32_t*)(&(*inst_pointer)[code_length])) = dword;
    code_length += 4;
}

inline void put16(uint16_t word)
{
    if ((code_length + 2) >= max_code_length)
    {
        max_code_length += JUMP_TABLE_SIZE;
        *inst_pointer = (unsigned char*)realloc_exec(*inst_pointer, code_length, max_code_length);
    }
    *((uint16_t*)(&(*inst_pointer)[code_length])) = word;
    code_length += 2;
}

void push_reg32(int32_t reg32)
{
    put8(0x50 + reg32);
}

void pop_reg32(int32_t reg32)
{
    put8(0x58 + reg32);
}

void mov_eax_memoffs32(void* _memoffs32)
{
    uint32_t* memoffs32 = (uint32_t*)_memoffs32;
    put8(0xA1);
    put32((uint32_t)(memoffs32));
}

void mov_memoffs32_eax(void* _memoffs32)
{
    uint32_t* memoffs32 = (uint32_t*)_memoffs32;
    put8(0xA3);
    put32((uint32_t)(memoffs32));
}

void mov_ax_memoffs16(uint16_t* memoffs16)
{
    put8(0x66);
    put8(0xA1);
    put32((uint32_t)(memoffs16));
}

void mov_memoffs16_ax(uint16_t* memoffs16)
{
    put8(0x66);
    put8(0xA3);
    put32((uint32_t)(memoffs16));
}

void mov_al_memoffs8(unsigned char* memoffs8)
{
    put8(0xA0);
    put32((uint32_t)(memoffs8));
}

void mov_memoffs8_al(unsigned char* memoffs8)
{
    put8(0xA2);
    put32((uint32_t)(memoffs8));
}

void mov_m8_imm8(unsigned char* m8, unsigned char imm8)
{
    put8(0xC6);
    put8(0x05);
    put32((uint32_t)(m8));
    put8(imm8);
}

void mov_m8_reg8(unsigned char* m8, int32_t reg8)
{
    put8(0x88);
    put8((reg8 << 3) | 5);
    put32((uint32_t)(m8));
}

void mov_reg16_m16(int32_t reg16, uint16_t* m16)
{
    put8(0x66);
    put8(0x8B);
    put8((reg16 << 3) | 5);
    put32((uint32_t)(m16));
}

void mov_m16_reg16(uint16_t* m16, int32_t reg16)
{
    put8(0x66);
    put8(0x89);
    put8((reg16 << 3) | 5);
    put32((uint32_t)(m16));
}

void cmp_reg32_m32(int32_t reg32, void* _m32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0x3B);
    put8((reg32 << 3) | 5);
    put32((uint32_t)(m32));
}

void cmp_reg32_reg32(int32_t reg1, int32_t reg2)
{
    put8(0x39);
    put8((reg2 << 3) | reg1 | 0xC0);
}

void cmp_reg32_imm8(int32_t reg32, unsigned char imm8)
{
    put8(0x83);
    put8(0xF8 + reg32);
    put8(imm8);
}

void cmp_preg32pimm32_imm8(int32_t reg32, uint32_t imm32, unsigned char imm8)
{
    put8(0x80);
    put8(0xB8 + reg32);
    put32(imm32);
    put8(imm8);
}

void cmp_reg32_imm32(int32_t reg32, uint32_t imm32)
{
    put8(0x81);
    put8(0xF8 + reg32);
    put32(imm32);
}

void test_reg32_imm32(int32_t reg32, uint32_t imm32)
{
    put8(0xF7);
    put8(0xC0 + reg32);
    put32(imm32);
}

void test_m32_imm32(void* _m32, uint32_t imm32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0xF7);
    put8(0x05);
    put32((uint32_t)m32);
    put32(imm32);
}

void test_al_imm8(unsigned char imm8)
{
    put8(0xA8);
    put8(imm8);
}

void cmp_al_imm8(unsigned char imm8)
{
    put8(0x3C);
    put8(imm8);
}

void add_m32_reg32(void* _m32, int32_t reg32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0x01);
    put8((reg32 << 3) | 5);
    put32((uint32_t)(m32));
}

void sub_reg32_m32(int32_t reg32, void* _m32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0x2B);
    put8((reg32 << 3) | 5);
    put32((uint32_t)(m32));
}

void sub_reg32_reg32(int32_t reg1, int32_t reg2)
{
    put8(0x29);
    put8((reg2 << 3) | reg1 | 0xC0);
}

void sbb_reg32_reg32(int32_t reg1, int32_t reg2)
{
    put8(0x19);
    put8((reg2 << 3) | reg1 | 0xC0);
}

void sub_reg32_imm32(int32_t reg32, uint32_t imm32)
{
    put8(0x81);
    put8(0xE8 + reg32);
    put32(imm32);
}

void sub_eax_imm32(uint32_t imm32)
{
    put8(0x2D);
    put32(imm32);
}

void jne_rj(unsigned char saut)
{
    put8(0x75);
    put8(saut);
}

void je_rj(unsigned char saut)
{
    put8(0x74);
    put8(saut);
}

void jb_rj(unsigned char saut)
{
    put8(0x72);
    put8(saut);
}

void jbe_rj(unsigned char saut)
{
    put8(0x76);
    put8(saut);
}

void ja_rj(unsigned char saut)
{
    put8(0x77);
    put8(saut);
}

void jae_rj(unsigned char saut)
{
    put8(0x73);
    put8(saut);
}

void jle_rj(unsigned char saut)
{
    put8(0x7E);
    put8(saut);
}

void jge_rj(unsigned char saut)
{
    put8(0x7D);
    put8(saut);
}

void jg_rj(unsigned char saut)
{
    put8(0x7F);
    put8(saut);
}

void jl_rj(unsigned char saut)
{
    put8(0x7C);
    put8(saut);
}

void jp_rj(unsigned char saut)
{
    put8(0x7A);
    put8(saut);
}

void je_near(uint32_t mi_addr)
{
    put8(0x0F);
    put8(0x84);
    put32(0);
    add_jump(code_length - 4, mi_addr);
}

void je_near_rj(uint32_t saut)
{
    put8(0x0F);
    put8(0x84);
    put32(saut);
}

void jl_near(uint32_t mi_addr)
{
    put8(0x0F);
    put8(0x8C);
    put32(0);
    add_jump(code_length - 4, mi_addr);
}

void jl_near_rj(uint32_t saut)
{
    put8(0x0F);
    put8(0x8C);
    put32(saut);
}

void jne_near(uint32_t mi_addr)
{
    put8(0x0F);
    put8(0x85);
    put32(0);
    add_jump(code_length - 4, mi_addr);
}

void jne_near_rj(uint32_t saut)
{
    put8(0x0F);
    put8(0x85);
    put32(saut);
}

void jge_near(uint32_t mi_addr)
{
    put8(0x0F);
    put8(0x8D);
    put32(0);
    add_jump(code_length - 4, mi_addr);
}

void jge_near_rj(uint32_t saut)
{
    put8(0x0F);
    put8(0x8D);
    put32(saut);
}

void jg_near(uint32_t mi_addr)
{
    put8(0x0F);
    put8(0x8F);
    put32(0);
    add_jump(code_length - 4, mi_addr);
}

void jle_near(uint32_t mi_addr)
{
    put8(0x0F);
    put8(0x8E);
    put32(0);
    add_jump(code_length - 4, mi_addr);
}

void jle_near_rj(uint32_t saut)
{
    put8(0x0F);
    put8(0x8E);
    put32(saut);
}

void mov_reg32_imm32(int32_t reg32, uint32_t imm32)
{
    put8(0xB8 + reg32);
    put32(imm32);
}

void jmp_imm(int32_t saut)
{
    put8(0xE9);
    put32(saut);
}

void jmp_imm_short(char saut)
{
    put8(0xEB);
    put8(saut);
}

void dec_reg32(int32_t reg32)
{
    put8(0x48 + reg32);
}

void inc_reg32(int32_t reg32)
{
    put8(0x40 + reg32);
}

void or_m32_imm32(void* _m32, uint32_t imm32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0x81);
    put8(0x0D);
    put32((uint32_t)(m32));
    put32(imm32);
}

void or_m32_reg32(void* _m32, uint32_t reg32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0x09);
    put8((reg32 << 3) | 5);
    put32((uint32_t)(m32));
}

void or_reg32_m32(uint32_t reg32, void* _m32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0x0B);
    put8((reg32 << 3) | 5);
    put32((uint32_t)(m32));
}

void or_reg32_reg32(uint32_t reg1, uint32_t reg2)
{
    put8(0x09);
    put8(0xC0 | (reg2 << 3) | reg1);
}

void and_reg32_reg32(uint32_t reg1, uint32_t reg2)
{
    put8(0x21);
    put8(0xC0 | (reg2 << 3) | reg1);
}

void and_m32_imm32(void* _m32, uint32_t imm32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0x81);
    put8(0x25);
    put32((uint32_t)(m32));
    put32(imm32);
}

void and_reg32_m32(uint32_t reg32, void* _m32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0x23);
    put8((reg32 << 3) | 5);
    put32((uint32_t)(m32));
}

void xor_reg32_reg32(uint32_t reg1, uint32_t reg2)
{
    put8(0x31);
    put8(0xC0 | (reg2 << 3) | reg1);
}

void xor_reg32_m32(uint32_t reg32, void* _m32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0x33);
    put8((reg32 << 3) | 5);
    put32((uint32_t)(m32));
}

void add_m32_imm32(void* _m32, uint32_t imm32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0x81);
    put8(0x05);
    put32((uint32_t)(m32));
    put32(imm32);
}

void add_m32_imm8(void* _m32, unsigned char imm8)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0x83);
    put8(0x05);
    put32((uint32_t)(m32));
    put8(imm8);
}

void sub_m32_imm32(void* _m32, uint32_t imm32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0x81);
    put8(0x2D);
    put32((uint32_t)(m32));
    put32(imm32);
}

void push_imm32(uint32_t imm32)
{
    put8(0x68);
    put32(imm32);
}

void add_reg32_imm8(uint32_t reg32, unsigned char imm8)
{
    put8(0x83);
    put8(0xC0 + reg32);
    put8(imm8);
}

void add_reg32_imm32(uint32_t reg32, uint32_t imm32)
{
    put8(0x81);
    put8(0xC0 + reg32);
    put32(imm32);
}

void inc_m32(void* _m32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0xFF);
    put8(0x05);
    put32((uint32_t)(m32));
}

void cmp_m32_imm32(void* _m32, uint32_t imm32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0x81);
    put8(0x3D);
    put32((uint32_t)(m32));
    put32(imm32);
}

void cmp_m32_imm8(void* _m32, unsigned char imm8)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0x83);
    put8(0x3D);
    put32((uint32_t)(m32));
    put8(imm8);
}

void cmp_m8_imm8(unsigned char* m8, unsigned char imm8)
{
    put8(0x80);
    put8(0x3D);
    put32((uint32_t)(m8));
    put8(imm8);
}

void cmp_eax_imm32(uint32_t imm32)
{
    put8(0x3D);
    put32(imm32);
}

void mov_m32_imm32(void* _m32, uint32_t imm32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0xC7);
    put8(0x05);
    put32((uint32_t)(m32));
    put32(imm32);
}

void jmp(uint32_t mi_addr)
{
    put8(0xE9);
    put32(0);
    add_jump(code_length - 4, mi_addr);
}

void cdq()
{
    put8(0x99);
}

void cwde()
{
    put8(0x98);
}

void cbw()
{
    put8(0x66);
    put8(0x98);
}

void mov_m32_reg32(void* _m32, uint32_t reg32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0x89);
    put8((reg32 << 3) | 5);
    put32((uint32_t)(m32));
}

void ret()
{
    put8(0xC3);
}

void call_reg32(uint32_t reg32)
{
    put8(0xFF);
    put8(0xD0 + reg32);
}

void call_m32(void* _m32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0xFF);
    put8(0x15);
    put32((uint32_t)(m32));
}

void shr_reg32_imm8(uint32_t reg32, unsigned char imm8)
{
    put8(0xC1);
    put8(0xE8 + reg32);
    put8(imm8);
}

void shr_reg32_cl(uint32_t reg32)
{
    put8(0xD3);
    put8(0xE8 + reg32);
}

void sar_reg32_cl(uint32_t reg32)
{
    put8(0xD3);
    put8(0xF8 + reg32);
}

void shl_reg32_cl(uint32_t reg32)
{
    put8(0xD3);
    put8(0xE0 + reg32);
}

void shld_reg32_reg32_cl(uint32_t reg1, uint32_t reg2)
{
    put8(0x0F);
    put8(0xA5);
    put8(0xC0 | (reg2 << 3) | reg1);
}

void shld_reg32_reg32_imm8(uint32_t reg1, uint32_t reg2, unsigned char imm8)
{
    put8(0x0F);
    put8(0xA4);
    put8(0xC0 | (reg2 << 3) | reg1);
    put8(imm8);
}

void shrd_reg32_reg32_cl(uint32_t reg1, uint32_t reg2)
{
    put8(0x0F);
    put8(0xAD);
    put8(0xC0 | (reg2 << 3) | reg1);
}

void sar_reg32_imm8(uint32_t reg32, unsigned char imm8)
{
    put8(0xC1);
    put8(0xF8 + reg32);
    put8(imm8);
}

void shrd_reg32_reg32_imm8(uint32_t reg1, uint32_t reg2, unsigned char imm8)
{
    put8(0x0F);
    put8(0xAC);
    put8(0xC0 | (reg2 << 3) | reg1);
    put8(imm8);
}

void mul_m32(void* _m32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0xF7);
    put8(0x25);
    put32((uint32_t)(m32));
}

void imul_m32(void* _m32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0xF7);
    put8(0x2D);
    put32((uint32_t)(m32));
}

void imul_reg32(uint32_t reg32)
{
    put8(0xF7);
    put8(0xE8 + reg32);
}

void mul_reg32(uint32_t reg32)
{
    put8(0xF7);
    put8(0xE0 + reg32);
}

void idiv_reg32(uint32_t reg32)
{
    put8(0xF7);
    put8(0xF8 + reg32);
}

void div_reg32(uint32_t reg32)
{
    put8(0xF7);
    put8(0xF0 + reg32);
}

void idiv_m32(void* _m32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0xF7);
    put8(0x3D);
    put32((uint32_t)(m32));
}

void div_m32(void* _m32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0xF7);
    put8(0x35);
    put32((uint32_t)(m32));
}

void add_reg32_reg32(uint32_t reg1, uint32_t reg2)
{
    put8(0x01);
    put8(0xC0 | (reg2 << 3) | reg1);
}

void adc_reg32_reg32(uint32_t reg1, uint32_t reg2)
{
    put8(0x11);
    put8(0xC0 | (reg2 << 3) | reg1);
}

void add_reg32_m32(uint32_t reg32, void* _m32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0x03);
    put8((reg32 << 3) | 5);
    put32((uint32_t)(m32));
}

void adc_reg32_m32(uint32_t reg32, void* _m32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0x13);
    put8((reg32 << 3) | 5);
    put32((uint32_t)(m32));
}

void adc_reg32_imm32(uint32_t reg32, uint32_t imm32)
{
    put8(0x81);
    put8(0xD0 + reg32);
    put32(imm32);
}

void jmp_reg32(uint32_t reg32)
{
    put8(0xFF);
    put8(0xE0 + reg32);
}

void jmp_m32(void* _m32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0xFF);
    put8(0x25);
    put32((uint32_t)(m32));
}

void mov_reg32_preg32(uint32_t reg1, uint32_t reg2)
{
    put8(0x8B);
    put8((reg1 << 3) | reg2);
}

void mov_preg32_reg32(int32_t reg1, int32_t reg2)
{
    put8(0x89);
    put8((reg2 << 3) | reg1);
}

void mov_reg32_preg32preg32pimm32(int32_t reg1, int32_t reg2, int32_t reg3, uint32_t imm32)
{
    put8(0x8B);
    put8((reg1 << 3) | 0x84);
    put8(reg2 | (reg3 << 3));
    put32(imm32);
}

void mov_reg32_preg32pimm32(int32_t reg1, int32_t reg2, uint32_t imm32)
{
    put8(0x8B);
    put8(0x80 | (reg1 << 3) | reg2);
    put32(imm32);
}

void mov_reg32_preg32x4preg32(int32_t reg1, int32_t reg2, int32_t reg3)
{
    put8(0x8B);
    put8((reg1 << 3) | 4);
    put8(0x80 | (reg2 << 3) | reg3);
}

void mov_reg32_preg32x4preg32pimm32(int32_t reg1, int32_t reg2, int32_t reg3, uint32_t imm32)
{
    put8(0x8B);
    put8((reg1 << 3) | 0x84);
    put8(0x80 | (reg2 << 3) | reg3);
    put32(imm32);
}

void mov_reg32_preg32x4pimm32(int32_t reg1, int32_t reg2, uint32_t imm32)
{
    put8(0x8B);
    put8((reg1 << 3) | 4);
    put8(0x80 | (reg2 << 3) | 5);
    put32(imm32);
}

void mov_preg32preg32pimm32_reg8(int32_t reg1, int32_t reg2, uint32_t imm32, int32_t reg8)
{
    put8(0x88);
    put8(0x84 | (reg8 << 3));
    put8((reg2 << 3) | reg1);
    put32(imm32);
}

void mov_preg32pimm32_reg8(int32_t reg32, uint32_t imm32, int32_t reg8)
{
    put8(0x88);
    put8(0x80 | reg32 | (reg8 << 3));
    put32(imm32);
}

void mov_preg32pimm32_imm8(int32_t reg32, uint32_t imm32, unsigned char imm8)
{
    put8(0xC6);
    put8(0x80 + reg32);
    put32(imm32);
    put8(imm8);
}

void mov_preg32pimm32_reg16(int32_t reg32, uint32_t imm32, int32_t reg16)
{
    put8(0x66);
    put8(0x89);
    put8(0x80 | reg32 | (reg16 << 3));
    put32(imm32);
}

void mov_preg32pimm32_reg32(int32_t reg1, uint32_t imm32, int32_t reg2)
{
    put8(0x89);
    put8(0x80 | reg1 | (reg2 << 3));
    put32(imm32);
}

void add_eax_imm32(uint32_t imm32)
{
    put8(0x05);
    put32(imm32);
}

void shl_reg32_imm8(uint32_t reg32, unsigned char imm8)
{
    put8(0xC1);
    put8(0xE0 + reg32);
    put8(imm8);
}

void mov_reg32_m32(uint32_t reg32, void* _m32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0x8B);
    put8((reg32 << 3) | 5);
    put32((uint32_t)(m32));
}

void mov_reg8_m8(int32_t reg8, unsigned char* m8)
{
    put8(0x8A);
    put8((reg8 << 3) | 5);
    put32((uint32_t)(m8));
}

void and_eax_imm32(uint32_t imm32)
{
    put8(0x25);
    put32(imm32);
}

void and_reg32_imm32(int32_t reg32, uint32_t imm32)
{
    put8(0x81);
    put8(0xE0 + reg32);
    put32(imm32);
}

void or_reg32_imm32(int32_t reg32, uint32_t imm32)
{
    put8(0x81);
    put8(0xC8 + reg32);
    put32(imm32);
}

void and_reg32_imm8(int32_t reg32, unsigned char imm8)
{
    put8(0x83);
    put8(0xE0 + reg32);
    put8(imm8);
}

void and_ax_imm16(uint16_t imm16)
{
    put8(0x66);
    put8(0x25);
    put16(imm16);
}

void and_al_imm8(unsigned char imm8)
{
    put8(0x24);
    put8(imm8);
}

void or_ax_imm16(uint16_t imm16)
{
    put8(0x66);
    put8(0x0D);
    put16(imm16);
}

void or_eax_imm32(uint32_t imm32)
{
    put8(0x0D);
    put32(imm32);
}

void xor_ax_imm16(uint16_t imm16)
{
    put8(0x66);
    put8(0x35);
    put16(imm16);
}

void xor_al_imm8(unsigned char imm8)
{
    put8(0x34);
    put8(imm8);
}

void xor_reg32_imm32(int32_t reg32, uint32_t imm32)
{
    put8(0x81);
    put8(0xF0 + reg32);
    put32(imm32);
}

void xor_reg8_imm8(int32_t reg8, unsigned char imm8)
{
    put8(0x80);
    put8(0xF0 + reg8);
    put8(imm8);
}

void nop()
{
    put8(0x90);
}

void mov_reg32_reg32(uint32_t reg1, uint32_t reg2)
{
    if (reg1 == reg2)
        return;
    put8(0x89);
    put8(0xC0 | (reg2 << 3) | reg1);
}

void not_reg32(uint32_t reg32)
{
    put8(0xF7);
    put8(0xD0 + reg32);
}

void movsx_reg32_m8(int32_t reg32, unsigned char* m8)
{
    put8(0x0F);
    put8(0xBE);
    put8((reg32 << 3) | 5);
    put32((uint32_t)(m8));
}

void movsx_reg32_reg8(int32_t reg32, int32_t reg8)
{
    put8(0x0F);
    put8(0xBE);
    put8((reg32 << 3) | reg8 | 0xC0);
}

void movsx_reg32_8preg32pimm32(int32_t reg1, int32_t reg2, uint32_t imm32)
{
    put8(0x0F);
    put8(0xBE);
    put8((reg1 << 3) | reg2 | 0x80);
    put32(imm32);
}

void movsx_reg32_16preg32pimm32(int32_t reg1, int32_t reg2, uint32_t imm32)
{
    put8(0x0F);
    put8(0xBF);
    put8((reg1 << 3) | reg2 | 0x80);
    put32(imm32);
}

void movsx_reg32_reg16(int32_t reg32, int32_t reg16)
{
    put8(0x0F);
    put8(0xBF);
    put8((reg32 << 3) | reg16 | 0xC0);
}

void movsx_reg32_m16(int32_t reg32, uint16_t* m16)
{
    put8(0x0F);
    put8(0xBF);
    put8((reg32 << 3) | 5);
    put32((uint32_t)(m16));
}

void fldcw_m16(uint16_t* m16)
{
    put8(0xD9);
    put8(0x2D);
    put32((uint32_t)(m16));
}

void fld_fpreg(int32_t fpreg)
{
    put8(0xD9);
    put8(0xC0 + fpreg);
}

void fld_preg32_dword(int32_t reg32)
{
    put8(0xD9);
    put8(reg32);
}

void fdiv_preg32_dword(int32_t reg32)
{
    put8(0xD8);
    put8(0x30 + reg32);
}

void fstp_fpreg(int32_t fpreg)
{
    put8(0xDD);
    put8(0xD8 + fpreg);
}

void fstp_preg32_dword(int32_t reg32)
{
    put8(0xD9);
    put8(0x18 + reg32);
}

void fldz()
{
    put8(0xD9);
    put8(0xEE);
}

void fchs()
{
    put8(0xD9);
    put8(0xE0);
}

void fstp_preg32_qword(int32_t reg32)
{
    put8(0xDD);
    put8(0x18 + reg32);
}

void fadd_preg32_dword(int32_t reg32)
{
    put8(0xD8);
    put8(reg32);
}

void fsub_preg32_dword(int32_t reg32)
{
    put8(0xD8);
    put8(0x20 + reg32);
}

void fmul_preg32_dword(int32_t reg32)
{
    put8(0xD8);
    put8(0x08 + reg32);
}

void fcomp_preg32_dword(int32_t reg32)
{
    put8(0xD8);
    put8(0x18 + reg32);
}

void fistp_preg32_dword(int32_t reg32)
{
    put8(0xDB);
    put8(0x18 + reg32);
}

void fistp_m32(void* _m32)
{
    uint32_t* m32 = (uint32_t*)_m32;
    put8(0xDB);
    put8(0x1D);
    put32((uint32_t)(m32));
}

void fistp_preg32_qword(int32_t reg32)
{
    put8(0xDF);
    put8(0x38 + reg32);
}

void fistp_m64(uint64_t* m64)
{
    put8(0xDF);
    put8(0x3D);
    put32((uint32_t)(m64));
}

void fld_preg32_qword(int32_t reg32)
{
    put8(0xDD);
    put8(reg32);
}

void fild_preg32_qword(int32_t reg32)
{
    put8(0xDF);
    put8(0x28 + reg32);
}

void fild_preg32_dword(int32_t reg32)
{
    put8(0xDB);
    put8(reg32);
}

void fadd_preg32_qword(int32_t reg32)
{
    put8(0xDC);
    put8(reg32);
}

void fdiv_preg32_qword(int32_t reg32)
{
    put8(0xDC);
    put8(0x30 + reg32);
}

void fsub_preg32_qword(int32_t reg32)
{
    put8(0xDC);
    put8(0x20 + reg32);
}

void fmul_preg32_qword(int32_t reg32)
{
    put8(0xDC);
    put8(0x08 + reg32);
}

void fsqrt()
{
    put8(0xD9);
    put8(0xFA);
}

void fabs_()
{
    put8(0xD9);
    put8(0xE1);
}

void fcomip_fpreg(int32_t fpreg)
{
    put8(0xDF);
    put8(0xF0 + fpreg);
}

void fucomi_fpreg(int32_t fpreg)
{
    put8(0xDB);
    put8(0xE8 + fpreg);
}

void fucomip_fpreg(int32_t fpreg)
{
    put8(0xDF);
    put8(0xE8 + fpreg);
}

void ffree_fpreg(int32_t fpreg)
{
    put8(0xDD);
    put8(0xC0 + fpreg);
}

void fclex()
{
    put8(0x9B);
    put8(0xDB);
    put8(0xE2);
}

void fstsw_ax()
{
    put8(0x9B);
    put8(0xDF);
    put8(0xE0);
}

void ud2()
{
    put8(0x0F);
    put8(0x0B);
}
