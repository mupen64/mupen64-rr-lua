/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include <Core.hpp>
#include <R4300/Macros.hpp>
#include <R4300/Recomph.hpp>
#include <R4300/x86_64/Assemble.hpp>
#include <R4300/x86_64/RegCache.hpp>
#include <Alloc.hpp>

typedef struct _jump_table
{
    uint32_t mi_addr;
    uint32_t pc_addr;
} jump_table;

static jump_table *jumps_table = NULL;
static int32_t jumps_number, max_jumps_number;

void init_assembler(void *block_jumps_table, int32_t block_jumps_number)
{
    if (block_jumps_table)
    {
        jumps_table = (jump_table *)block_jumps_table;
        jumps_number = block_jumps_number;
        max_jumps_number = jumps_number;
    }
    else
    {
        jumps_table = (jump_table *)malloc(JUMP_TABLE_SIZE * sizeof(jump_table));
        jumps_number = 0;
        max_jumps_number = JUMP_TABLE_SIZE;
    }
}

void free_assembler(void **block_jumps_table, int32_t *block_jumps_number)
{
    *block_jumps_table = jumps_table;
    *block_jumps_number = jumps_number;
}

static void add_jump(uint32_t pc_addr, uint32_t mi_addr)
{
    if (jumps_number == max_jumps_number)
    {
        max_jumps_number += JUMP_TABLE_SIZE;
        jumps_table = (jump_table *)realloc(jumps_table, max_jumps_number * sizeof(jump_table));
    }
    jumps_table[jumps_number].pc_addr = pc_addr;
    jumps_table[jumps_number].mi_addr = mi_addr;
    jumps_number++;
}

#define X64_JUMP_STUB_REG 2

void passe2(precomp_instr *dest, int32_t start, int32_t end, precomp_block *block)
{
    uint32_t i, real_code_length;
    uintptr_t addr_dest;
    build_wrappers(dest, start, end, block);
    real_code_length = code_length;

    for (i = 0; i < jumps_number; i++)
    {
        code_length = jumps_table[i].pc_addr;
        uintptr_t jmp_pc = (uintptr_t)block->code + code_length;
        uintptr_t jmp_next = jmp_pc + 4;

        if (dest[(jumps_table[i].mi_addr - dest[0].addr) / 4].reg_cache_infos.need_map)
        {
            addr_dest = (uintptr_t)&dest[(jumps_table[i].mi_addr - dest[0].addr) / 4].reg_cache_infos.jump_wrapper[0];
            uintptr_t stub_addr = (uintptr_t)block->code + real_code_length;
            int32_t disp = (int32_t)(stub_addr - jmp_next);
            put32((uint32_t)disp);
            code_length = real_code_length;
            mov_reg64_imm64(X64_JUMP_STUB_REG, addr_dest);
            jmp_reg64(X64_JUMP_STUB_REG);
            real_code_length = code_length;
        }
        else
        {
            uintptr_t target = (uintptr_t)block->code
                + (uintptr_t)dest[(jumps_table[i].mi_addr - dest[0].addr) / 4].local_addr;
            int32_t disp = (int32_t)(target - jmp_next);
            put32((uint32_t)disp);
        }
    }
    code_length = real_code_length;
}

void debug()
{
}

inline void grow_buffer()
{
    // Copy the FULL old allocation, not the transient `code_length`. During
    // passe2's jump-patching loop, code_length is temporarily rewound to a jump
    // site (a small mid-buffer offset) before each put32. If a grow fires while
    // code_length is rewound, copying only `code_length` bytes would truncate
    // all real code/stubs emitted past that offset, zeroing the buffer tail and
    // leaving instructions whose local_addr points there jumping into zeros.
    // The buffer holds up to `max_code_length` (old) valid bytes, so copy that.
    size_t old_size = max_code_length;
    max_code_length += JUMP_TABLE_SIZE;
    unsigned char *new_buffer = (unsigned char *)realloc_exec(*inst_pointer, old_size, max_code_length);
    if (!new_buffer)
    {
        g_core->log_error(std::format(
            "[Dynarec] FATAL: code buffer growth failed (code_length={}, max_code_length={})",
            code_length, max_code_length));
        abort();
    }
    *inst_pointer = new_buffer;
}

inline void put8(unsigned char octet)
{
    if (code_length == max_code_length)
    {
        grow_buffer();
    }
    (*inst_pointer)[code_length] = octet;
    code_length++;
}

inline void put32(uint32_t dword)
{
    if ((code_length + 4) >= max_code_length)
    {
        grow_buffer();
    }
    *((uint32_t *)(&(*inst_pointer)[code_length])) = dword;
    code_length += 4;
}

inline void put64(uint64_t qword)
{
    if ((code_length + 8) >= max_code_length)
    {
        grow_buffer();
    }
    *((uint64_t *)(&(*inst_pointer)[code_length])) = qword;
    code_length += 8;
}

inline void put16(uint16_t word)
{
    if ((code_length + 2) >= max_code_length)
    {
        grow_buffer();
    }
    *((uint16_t *)(&(*inst_pointer)[code_length])) = word;
    code_length += 2;
}

// x64 helper: load a 64-bit immediate into R11 (scratch register)
static void mov_r11_imm64(uintptr_t imm64)
{
    put8(0x49);       // REX.W=1, REX.B=1
    put8(0xB8 + 3);   // mov r11, imm64
    put64(imm64);
}

void push_reg32(int32_t reg32)
{
    put8(0x50 + reg32);
}

void pop_reg32(int32_t reg32)
{
    put8(0x58 + reg32);
}

void mov_eax_memoffs32(void *_memoffs32)
{
    mov_r11_imm64((uintptr_t)(_memoffs32));
    put8(0x41);
    put8(0x8B);
    put8(0x03);
}

void mov_memoffs32_eax(void *_memoffs32)
{
    mov_r11_imm64((uintptr_t)(_memoffs32));
    put8(0x41);
    put8(0x89);
    put8(0x03);
}

void mov_ax_memoffs16(uint16_t *memoffs16)
{
    mov_r11_imm64((uintptr_t)(memoffs16));
    put8(0x66);
    put8(0x41);
    put8(0x8B);
    put8(0x03);
}

void mov_memoffs16_ax(uint16_t *memoffs16)
{
    mov_r11_imm64((uintptr_t)(memoffs16));
    put8(0x66);
    put8(0x41);
    put8(0x89);
    put8(0x03);
}

void mov_al_memoffs8(unsigned char *memoffs8)
{
    mov_r11_imm64((uintptr_t)(memoffs8));
    put8(0x41);
    put8(0x8A);
    put8(0x03);
}

void mov_memoffs8_al(unsigned char *memoffs8)
{
    mov_r11_imm64((uintptr_t)(memoffs8));
    put8(0x41);
    put8(0x88);
    put8(0x03);
}

void mov_m8_imm8(unsigned char *m8, unsigned char imm8)
{
    mov_r11_imm64((uintptr_t)(m8));
    put8(0x41);
    put8(0xC6);
    put8(0x03);
    put8(imm8);
}

void mov_m8_reg8(unsigned char *m8, int32_t reg8)
{
    mov_r11_imm64((uintptr_t)(m8));
    put8(0x41);
    put8(0x88);
    put8((reg8 << 3) | 0x03);
}

void mov_reg16_m16(int32_t reg16, uint16_t *m16)
{
    mov_r11_imm64((uintptr_t)(m16));
    put8(0x66);
    put8(0x41);
    put8(0x8B);
    put8((reg16 << 3) | 0x03);
}

void mov_m16_reg16(uint16_t *m16, int32_t reg16)
{
    mov_r11_imm64((uintptr_t)(m16));
    put8(0x66);
    put8(0x41);
    put8(0x89);
    put8((reg16 << 3) | 0x03);
}

void cmp_reg32_m32(int32_t reg32, void *_m32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0x3B);
    put8((reg32 << 3) | 0x03);
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

// x64: cmp byte [r11], imm8 — after mov_r11_imm64 + add r11, reg32
void cmp_preg32pimm32_imm8_r11(int32_t reg32, uintptr_t addr64, unsigned char imm8)
{
    mov_r11_imm64(addr64);
    // add r11, reg32 (64-bit)
    put8(0x49 | (((reg32 >> 3) & 1) << 2));
    put8(0x01);
    put8(0xC0 | ((reg32 & 7) << 3) | 3);
    // cmp byte [r11], imm8
    put8(0x41);
    put8(0x80);
    put8(0x3B);
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

void test_m32_imm32(void *_m32, uint32_t imm32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0xF7);
    put8(0x03);
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

void add_m32_reg32(void *_m32, int32_t reg32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0x01);
    put8((reg32 << 3) | 0x03);
}

void sub_reg32_m32(int32_t reg32, void *_m32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0x2B);
    put8((reg32 << 3) | 0x03);
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

void rj_patch(int32_t disp_pos)
{
    int32_t rel = code_length - (disp_pos + 1);
    // rel8 range check: forward short jumps must fit in a signed byte.
    if (rel < -128 || rel > 127)
    {
        g_core->log_error(std::format(
            "[Dynarec] FATAL: rj_patch rel8 out of range ({}) at disp_pos={}", rel, disp_pos));
        abort();
    }
    (*inst_pointer)[disp_pos] = (unsigned char)(int8_t)rel;
}

void rj_patch_near(int32_t disp_pos)
{
    // Patch a rel32 displacement (near jcc/jmp) emitted with a placeholder.
    // `disp_pos` is the offset of the 4-byte displacement field.
    int32_t rel = code_length - (disp_pos + 4);
    unsigned char *p = &(*inst_pointer)[disp_pos];
    p[0] = (unsigned char)(rel & 0xFF);
    p[1] = (unsigned char)((rel >> 8) & 0xFF);
    p[2] = (unsigned char)((rel >> 16) & 0xFF);
    p[3] = (unsigned char)((rel >> 24) & 0xFF);
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

void mov_reg64_imm64(int32_t reg32, uintptr_t imm64)
{
    put8(0x48 | ((reg32 >> 3) & 1));
    put8(0xB8 + (reg32 & 7));
    put64(imm64);
}

// x64 helpers for full 64-bit pointer arithmetic / loads. The dynarec's jr/jalr
// same-page fast path must manipulate 64-bit pointers (block/code buffers are
// allocated above 4GB on x64); using the 32-bit emitters truncates the high
// half and makes the computed jump land in a non-executable low address.

void add_reg64_reg64(uint32_t reg1, uint32_t reg2)
{
    // add r64, r64  (REX.W, opcode 0x01 /r, mod=11)
    put8(0x48 | (((reg2 >> 3) & 1) << 2) | ((reg1 >> 3) & 1));
    put8(0x01);
    put8(0xC0 | ((reg2 & 7) << 3) | (reg1 & 7));
}

void mov_reg32_preg64(int32_t dst, int32_t base)
{
    // mov r32, [r64]  (32-bit load, 64-bit base, mod=00). base must not be RSP/RBP.
    unsigned char rex = 0x40 | (((dst >> 3) & 1) << 2) | ((base >> 3) & 1);
    if (rex != 0x40) put8(rex);
    put8(0x8B);
    put8(((dst & 7) << 3) | (base & 7));
}

void mov_reg64_preg64(int32_t dst, int32_t base)
{
    // mov r64, [r64]  (64-bit load, 64-bit base, mod=00). base must not be RSP/RBP.
    put8(0x48 | (((dst >> 3) & 1) << 2) | ((base >> 3) & 1));
    put8(0x8B);
    put8(((dst & 7) << 3) | (base & 7));
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
    put8(0x48 | ((reg32 >> 3) & 1));
    put8(0xFF);
    put8(0xC8 | (reg32 & 7));
}

void inc_reg32(int32_t reg32)
{
    // In 64-bit mode, 0x40+reg are REX prefixes, not INC.
    // Use FF /0 (inc r/m) with REX.W for proper 64-bit INC.
    put8(0x48 | ((reg32 >> 3) & 1));
    put8(0xFF);
    put8(0xC0 | (reg32 & 7));
}

void or_m32_imm32(void *_m32, uint32_t imm32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0x81);
    put8(0x0B);
    put32(imm32);
}

void or_m32_reg32(void *_m32, uint32_t reg32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0x09);
    put8((reg32 << 3) | 0x03);
}

void or_reg32_m32(uint32_t reg32, void *_m32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0x0B);
    put8((reg32 << 3) | 0x03);
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

void and_m32_imm32(void *_m32, uint32_t imm32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0x81);
    put8(0x23);
    put32(imm32);
}

void and_reg32_m32(uint32_t reg32, void *_m32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0x23);
    put8((reg32 << 3) | 0x03);
}

void xor_reg32_reg32(uint32_t reg1, uint32_t reg2)
{
    put8(0x31);
    put8(0xC0 | (reg2 << 3) | reg1);
}

void xor_reg32_m32(uint32_t reg32, void *_m32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0x33);
    put8((reg32 << 3) | 0x03);
}

void add_m32_imm32(void *_m32, uint32_t imm32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0x81);
    put8(0x03);
    put32(imm32);
}

void add_m32_imm8(void *_m32, unsigned char imm8)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0x83);
    put8(0x03);
    put8(imm8);
}

void sub_m32_imm32(void *_m32, uint32_t imm32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0x81);
    put8(0x2B);
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

void inc_m32(void *_m32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0xFF);
    put8(0x03);
}

void cmp_m32_imm32(void *_m32, uint32_t imm32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0x81);
    put8(0x3B);
    put32(imm32);
}

void cmp_m32_imm8(void *_m32, unsigned char imm8)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0x83);
    put8(0x3B);
    put8(imm8);
}

void cmp_m8_imm8(unsigned char *m8, unsigned char imm8)
{
    mov_r11_imm64((uintptr_t)(m8));
    put8(0x41);
    put8(0x80);
    put8(0x3B);
    put8(imm8);
}

void cmp_eax_imm32(uint32_t imm32)
{
    put8(0x3D);
    put32(imm32);
}

void cmp_eax_r11()
{
    // REX.W=1, REX.B=1 for r11
    put8(0x49);
    put8(0x39);
    put8(0xC3); // cmp eax, r11
}

void cmp_rax_r11()
{
    // REX.W=1, REX.B=1 for r11 ; cmp rax, r11
    put8(0x49);
    put8(0x3B);
    put8(0xC3);
}

// mov rax, [readmemb + index_reg*8] - load 64-bit function pointer from dispatch table
// Uses R11 as scratch to hold the base address
void mov_rax_preg32x4pimm32(int32_t dst_reg, int32_t idx_reg, uintptr_t base_addr)
{
    // mov r11, base_addr (10 bytes)
    mov_r11_imm64(base_addr);
    // mov rax, [r11 + idx_reg*8] with REX.WB + 8B /r + SIB
    // REX.WB = 0x49 (REX.W=1, REX.B=1 for r11)
    // 8B /r : mod=00, reg=dst_reg (rax=0), r/m=SIB=4 → 0x04 | (dst_reg << 3)
    // SIB : scale=3 (x8), index=idx_reg, base=7 (r11) → scale=3 means 0xC0
    put8(0x49);                    // REX.WB
    put8(0x8B);
    put8(0x04 | (dst_reg << 3));   // mod=00, reg=rax, r/m=SIB(4)
    put8(0xC0 | (idx_reg << 3) | 7); // SIB: scale=3 (x8), index=idx_reg, base=r11(7)
}

void mov_rax_r11_idx4(int32_t idx_reg)
{
    // mov rax, [r11 + idx*8] - used after mov_r11_imm64(base)
    // REX.WB = 0x49 (W=1 for rax, B=1 for r11)
    put8(0x49);
    put8(0x8B);
    put8(0x04);                  // mod=00, reg=0 (rax), r/m=SIB(4)
    put8(0xC0 | (idx_reg << 3) | 7); // SIB: scale=3 (x8), index=idx_reg, base=r11
}

void cmp_rax_imm64(uintptr_t imm64)
{
    mov_r11_imm64(imm64);
    cmp_rax_r11();
}

void mov_m32_imm32(void *_m32, uint32_t imm32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0xC7);
    put8(0x03);
    put32(imm32);
}

void mov_m64_imm64(void *_m64, uintptr_t imm64)
{
    mov_r11_imm64((uintptr_t)(_m64));
    mov_reg64_imm64(EAX, imm64);
    put8(0x49);
    put8(0x89);
    put8(0x03); // mov [r11], rax
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

void mov_m32_reg32(void *_m32, uint32_t reg32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0x89);
    put8((reg32 << 3) | 0x03);
}

// 64-bit store: mov [mem], reg64 (full 64-bit register save)
void mov_m64_reg64(void *_m64, uint32_t reg64)
{
    mov_r11_imm64((uintptr_t)(_m64));
    // REX.W=1 for 64-bit operand, REX.B=1 for r11 base
    put8(0x49);                          // REX.WB
    put8(0x89);                          // mov r/m64, r64
    put8((reg64 << 3) | 0x03);           // mod=00, reg=reg64, r/m=011 (r11)
}

// 64-bit load: mov reg64, [mem] (full 64-bit register load)
void mov_reg64_m64(uint32_t reg64, void *_m64)
{
    mov_r11_imm64((uintptr_t)(_m64));
    // REX.W=1 for 64-bit operand, REX.B=1 for r11 base
    put8(0x49);                          // REX.WB
    put8(0x8B);                          // mov r64, r/m64
    put8((reg64 << 3) | 0x03);           // mod=00, reg=reg64, r/m=011 (r11)
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

// 64-bit call: in long mode, FF /2 (CALL r/m64) uses the full 64-bit
// register by default — no REX.W needed. REX.B only extends the r/m field
// to access r8-r15. Emitting REX.B unconditionally would shift regs 0-7
// to r8-r15 (e.g. EAX=0 → r8 instead of rax), causing calls to wild addresses.
static void call_reg64_impl(uint32_t reg32)
{
    uint8_t rm = reg32 & 7;
    if (reg32 & 8)
        put8(0x41);                             // REX.B for r8-r15
    put8(0xFF);
    put8(0xD0 + rm);
}

void call_reg64(uint32_t reg32)
{
    // x64 ABI: RSP must be 16-byte aligned before a CALL instruction so the
    // callee sees RSP%16==8 after CALL pushes the return address.
    //
    // This emitted code runs with RSP%16==8 on entry (both from the NOTCOMPILED
    // stub and from previously-executed dynarec code). push r11 both saves the
    // caller's r11 and aligns RSP from 8mod16 to 0mod16, so the subsequent CALL
    // lands the callee at RSP%16==8 as required by the ABI.
    //
    // We also publish the address where CALL will write its return address
    // (8 bytes below the push, i.e. RSP-8) to the global return_address so
    // dyna_jump can repoint it. After a normal return we just pop r11.
    //
    // We use rcx as scratch for the return-address slot computation.
    // R10 is reserved for the dynarec's lookup-table base pointer
    // (see mov_reg32_preg32x4_r10) and must not be clobbered here.
    // RCX is caller-saved (volatile) and never used as the call_reg64
    // target register (callers always pass EAX or EBX), so it is safe.
    //
    // Stack layout (entry RSP%16==8, X := entry RSP):
    //   push r11           ; RSP = X-8, RSP%16==0 (save r11 + align for CALL)
    //   lea rcx, [rsp - 8] ; rcx = X-16, where CALL will store the retaddr
    //   mov r11, imm64(&return_address)
    //   mov [r11], rcx     ; publish the retaddr slot location
    //   call reg32         ; RSP = X-16 inside callee (ABI-correct)
    // normal return:
    //   pop r11            ; RSP = X (restored)

    // push r11 (REX.B 0x53)
    put8(0x41);
    put8(0x53);

    // lea rcx, [rsp - 8] -> rcx = address where CALL will write the retaddr
    // REX.W=1 (64-bit operand) => 0x48
    // opcode 0x8D (lea)
    // ModR/M: mod=01, reg=001 (rcx), r/m=100 (SIB)
    // SIB=0x24 (scale=00, index=100 none, base=100 RSP), disp8=0xF8 (-8)
    put8(0x48);
    put8(0x8D);
    put8(0x4C);
    put8(0x24);
    put8(0xF8);

    // mov r11, imm64(&return_address)
    mov_r11_imm64((uintptr_t)&return_address);

    // mov [r11], rcx   (publish the retaddr-slot address)
    // REX.W=1, REX.B=1 (r11 dest) => 0x49
    // opcode 0x89, ModR/M: mod=00, reg=001 (rcx), r/m=011 (r11 with B)
    put8(0x49);
    put8(0x89);
    put8(0x0B);

    // call reg64 (64-bit operand using REX.B prefix)
    call_reg64_impl(reg32);

    // restore r11
    // pop r11 (REX.B 0x5B)
    put8(0x41);
    put8(0x5B);
}

void call_m32(void *_m32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0xFF);
    put8(0x13);
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

void mul_m32(void *_m32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0xF7);
    put8(0x23);
}

void imul_m32(void *_m32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0xF7);
    put8(0x2B);
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

void idiv_m32(void *_m32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0xF7);
    put8(0x3B);
}

void div_m32(void *_m32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0xF7);
    put8(0x33);
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

void add_reg32_m32(uint32_t reg32, void *_m32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0x03);
    put8((reg32 << 3) | 0x03);
}

void adc_reg32_m32(uint32_t reg32, void *_m32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0x13);
    put8((reg32 << 3) | 0x03);
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
    put8(0xE0 + (reg32 & 7));
}

void jmp_reg64(uint32_t reg64)
{
    put8(0x48 | ((reg64 >> 3) & 1));
    put8(0xFF);
    put8(0xE0 + (reg64 & 7));
}

void jmp_m32(void *_m32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0xFF);
    put8(0x23);
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

// x64: mov reg1, [r11] — after mov_r11_imm64(addr64) + add r11, reg2
void mov_reg32_preg32pimm32_r11(int32_t reg1, int32_t reg2, uintptr_t addr64)
{
    mov_r11_imm64(addr64);
    // add r11, reg2 (64-bit): opcode 0x01 (ADD r/m64, r64) with r/m=r11, reg=reg2.
    // REX.W=1, REX.B=1 (r/m is r11), REX.R = high bit of reg2. The old encoding
    // used 0x4C (REX.W|REX.R) with B from reg2 — R and B were swapped, so for the
    // usual reg2 in 0-7 it emitted `add rbx, r11` and r11 (the rdram base) never
    // advanced: every fast-path RDRAM load read rdram[0] instead of rdram[offset].
    put8(0x49 | (((reg2 >> 3) & 1) << 2));
    put8(0x01);
    put8(0xC0 | ((reg2 & 7) << 3) | 3);
    // mov reg1, [r11]
    put8(0x41);
    put8(0x8B);
    put8((reg1 << 3) | 0x03);
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

// x64: load a 64-bit function pointer into R10, then dereference [index*4 + R10].
// This replaces mov_reg32_preg32x4pimm32 which truncates 64-bit pointers to 32 bits
// in the instruction's disp32 field, causing crashes when function pointers live
// above 4 GB. R10 is reserved exclusively for lookup-table base pointers in
// the dynarec. call_reg64 uses RCX as scratch (not R10) to avoid clobbering
// the table base between mov_reg32_preg32x4_r10 and the subsequent CALL.
void mov_reg32_preg32x4_r10(int32_t dst_reg, int32_t index_reg, uint64_t func_ptr)
{
    // mov r10, imm64  (REX.WB=1: W=1 for 64-bit, B=1 extends reg field to R10) => 0x49, opcode 0xB8+2=0xBA)
    put8(0x49);
    put8(0xBA);
    put64(func_ptr);

    // mov dst_reg, [index_reg*8 + r10]
    // REX.WB=1 (r10 base + 64-bit operand) => 0x49
    // opcode 0x8B, ModR/M: mod=00, reg=dst, r/m=100(SIB)
    put8(0x49);
    put8(0x8B);
    put8((dst_reg << 3) | 4);
    // SIB: scale=3 (index*8), index=index_reg, base=R10 (reg2=2, REX.B extends to R10)
    put8((3 << 6) | (index_reg << 3) | 2);
}

// Like above but also emit a comparison against a 64-bit value held in R11.
// Used for the fast-memory path check (e.g. checking if accessed memory is in RDRAM
// by comparing the loaded function pointer against read_rdramb stored in R11).
// R11 is used here because call_reg64 saves/restores it, so it is pre-populated
// by the caller with the comparison value.
void cmp_reg32_prx4_r10(int32_t index_reg, uint64_t func_ptr)
{
    // mov r10, imm64  (REX.WB=1 => 0x49, opcode 0xB8+2=0xBA)
    put8(0x49);
    put8(0xBA);
    put64(func_ptr);

    // mov r11, [index_reg*8 + r10]
    // REX.WRB=1 => 0x4D (W=1 for 64-bit, R=1 for R11, B=1 for R10)
    // opcode 0x8B, ModR/M: mod=00, reg=11(R11), r/m=100(SIB)
    put8(0x4D);
    put8(0x8B);
    put8((3 << 3) | 4);
    // SIB: scale=3 (index*8), index=index_reg, base=R10
    put8((3 << 6) | (index_reg << 3) | 2);
}

// x64: mov with scaled-indexed addressing using a 64-bit pointer embedded as an
// immediate. Since x86 does not support 64-bit disp in ModR/M, we load the pointer
// into R10 then emit [index*4+R10] indexed dereference — same as mov_reg32_preg32x4_r10.
void mov_reg32_preg32x4pimm64(int32_t dst_reg, int32_t index_reg, uint64_t ptr)
{
    // mov r10, imm64  (REX.WB=1 => 0x49, opcode 0xB8+2=0xBA)
    put8(0x49);
    put8(0xBA);
    put64(ptr);

    // mov dst_reg, [index_reg*8 + r10]
    // REX.WB=1 (64-bit operand + r10 base) => 0x49
    // opcode 0x8B, ModR/M: mod=00, reg=dst, r/m=100(SIB)
    put8(0x49);
    put8(0x8B);
    put8((dst_reg << 3) | 4);
    // SIB: scale=3 (index*8), index=index_reg, base=R10 (reg2=2, REX.B=1 => R10)
    put8((3 << 6) | (index_reg << 3) | 2);
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

// x64: mov [r11], reg8 — after mov_r11_imm64(addr64) + add r11, reg32
void mov_preg32pimm32_reg8_r11(int32_t reg32, uintptr_t addr64, int32_t reg8)
{
    mov_r11_imm64(addr64);
    // add r11, reg32 (64-bit)
    put8(0x49 | (((reg32 >> 3) & 1) << 2));
    put8(0x01);
    put8(0xC0 | ((reg32 & 7) << 3) | 3);
    // mov [r11], reg8
    put8(0x41);
    put8(0x88);
    put8((reg8 << 3) | 0x03);
}

void mov_preg32pimm32_imm8(int32_t reg32, uint32_t imm32, unsigned char imm8)
{
    put8(0xC6);
    put8(0x80 + reg32);
    put32(imm32);
    put8(imm8);
}

// x64: mov byte [r11], imm8 — after mov_r11_imm64(addr64) + add r11, reg32
void mov_preg32pimm32_imm8_r11(int32_t reg32, uintptr_t addr64, unsigned char imm8)
{
    mov_r11_imm64(addr64);
    // add r11, reg32 (64-bit)
    put8(0x49 | (((reg32 >> 3) & 1) << 2));
    put8(0x01);
    put8(0xC0 | ((reg32 & 7) << 3) | 3);
    // mov byte [r11], imm8
    put8(0x41);
    put8(0xC6);
    put8(0x03);
    put8(imm8);
}

void mov_preg32pimm32_reg16(int32_t reg32, uint32_t imm32, int32_t reg16)
{
    put8(0x66);
    put8(0x89);
    put8(0x80 | reg32 | (reg16 << 3));
    put32(imm32);
}

// x64: mov word [r11], reg16 — after mov_r11_imm64(addr64) + add r11, reg32
void mov_preg32pimm32_reg16_r11(int32_t reg32, uintptr_t addr64, int32_t reg16)
{
    mov_r11_imm64(addr64);
    // add r11, reg32 (64-bit)
    put8(0x49 | (((reg32 >> 3) & 1) << 2));
    put8(0x01);
    put8(0xC0 | ((reg32 & 7) << 3) | 3);
    // mov word [r11], reg16
    put8(0x66);
    put8(0x41);
    put8(0x89);
    put8((reg16 << 3) | 0x03);
}

void mov_preg32pimm32_reg32(int32_t reg1, uint32_t imm32, int32_t reg2)
{
    put8(0x89);
    put8(0x80 | reg1 | (reg2 << 3));
    put32(imm32);
}

// x64: mov [r11], reg2 — after mov_r11_imm64(addr64) + add r11, reg1
void mov_preg32pimm32_reg32_r11(int32_t reg1, uintptr_t addr64, int32_t reg2)
{
    mov_r11_imm64(addr64);
    // add r11, reg1 (64-bit)
    put8(0x49 | (((reg1 >> 3) & 1) << 2));
    put8(0x01);
    put8(0xC0 | ((reg1 & 7) << 3) | 3);
    // mov [r11], reg2
    put8(0x41);
    put8(0x89);
    put8((reg2 << 3) | 0x03);
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

void mov_reg32_m32(uint32_t reg32, void *_m32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0x8B);
    put8((reg32 << 3) | 0x03);
}

void mov_reg8_m8(int32_t reg8, unsigned char *m8)
{
    mov_r11_imm64((uintptr_t)(m8));
    put8(0x41);
    put8(0x8A);
    put8((reg8 << 3) | 0x03);
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
    if (reg1 == reg2) return;
    put8(0x89);
    put8(0xC0 | (reg2 << 3) | reg1);
}

// Sign-extend a 32-bit register into a full 64-bit register: movsxd dst64, src32.
// Needed because 32-bit x86 ALU ops zero-extend into the upper 32 bits, but a
// 32-bit MIPS result must be sign-extended when written back to the 64-bit reg[]
// slot (see free_register / force_32 in RegCache.cpp).
void movsxd_reg64_reg32(uint32_t dst, uint32_t src)
{
    // REX.W (0x48) + REX.R (dst>=8) + REX.B (src>=8)
    put8(0x48 | ((dst & 8) >> 1) | ((src & 8) >> 3));
    put8(0x63); // MOVSXD r64, r/m32
    put8(0xC0 | ((dst & 7) << 3) | (src & 7));
}

void not_reg32(uint32_t reg32)
{
    put8(0xF7);
    put8(0xD0 + reg32);
}

void movsx_reg32_m8(int32_t reg32, unsigned char *m8)
{
    mov_r11_imm64((uintptr_t)(m8));
    put8(0x41);
    put8(0x0F);
    put8(0xBE);
    put8((reg32 << 3) | 0x03);
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

// x64: movsx reg1, byte [r11] — after mov_r11_imm64(addr64) + add r11, reg2
void movsx_reg32_8preg32pimm32_r11(int32_t reg1, int32_t reg2, uintptr_t addr64)
{
    mov_r11_imm64(addr64);
    // add r11, reg2 (64-bit)
    put8(0x49 | (((reg2 >> 3) & 1) << 2));
    put8(0x01);
    put8(0xC0 | ((reg2 & 7) << 3) | 3);
    // movsx reg1, byte [r11]
    put8(0x41);
    put8(0x0F);
    put8(0xBE);
    put8((reg1 << 3) | 0x03);
}

void movsx_reg32_16preg32pimm32(int32_t reg1, int32_t reg2, uint32_t imm32)
{
    put8(0x0F);
    put8(0xBF);
    put8((reg1 << 3) | reg2 | 0x80);
    put32(imm32);
}

// x64: movsx reg1, word [r11] — after mov_r11_imm64(addr64) + add r11, reg2
void movsx_reg32_16preg32pimm32_r11(int32_t reg1, int32_t reg2, uintptr_t addr64)
{
    mov_r11_imm64(addr64);
    // add r11, reg2 (64-bit)
    put8(0x49 | (((reg2 >> 3) & 1) << 2));
    put8(0x01);
    put8(0xC0 | ((reg2 & 7) << 3) | 3);
    // movsx reg1, word [r11]
    put8(0x41);
    put8(0x0F);
    put8(0xBF);
    put8((reg1 << 3) | 0x03);
}

void movsx_reg32_reg16(int32_t reg32, int32_t reg16)
{
    put8(0x0F);
    put8(0xBF);
    put8((reg32 << 3) | reg16 | 0xC0);
}

void movsx_reg32_m16(int32_t reg32, uint16_t *m16)
{
    mov_r11_imm64((uintptr_t)(m16));
    put8(0x41);
    put8(0x0F);
    put8(0xBF);
    put8((reg32 << 3) | 0x03);
}

void fldcw_m16(uint16_t *m16)
{
    mov_r11_imm64((uintptr_t)(m16));
    put8(0x41);
    put8(0xD9);
    put8(0x2B);
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

void fistp_m32(void *_m32)
{
    mov_r11_imm64((uintptr_t)(_m32));
    put8(0x41);
    put8(0xDB);
    put8(0x1B);
}

void fistp_preg32_qword(int32_t reg32)
{
    put8(0xDF);
    put8(0x38 + reg32);
}

void fistp_m64(uint64_t *m64)
{
    mov_r11_imm64((uintptr_t)(m64));
    put8(0x41);
    put8(0xDF);
    put8(0x3B);
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

