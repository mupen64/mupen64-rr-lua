/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Core.hpp>
#include <R4300/Macros.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Recomph.hpp>
#include <R4300/arm64/Assemble.hpp>
#include <R4300/arm64/RegCache.hpp>
#include <Alloc.hpp>

int32_t branch_taken = 0;

typedef struct _jump_table
{
    uint32_t mi_addr;
    uint32_t pc_addr;
} jump_table;

static jump_table *jumps_table = NULL;
static int32_t jumps_number, max_jumps_number;

void init_assembler(void *block_jumps_table, int32_t block_jumps_number)
{
    exec_write_begin();

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

    if (inst_pointer && *inst_pointer && code_length > 0)
    {
        __builtin___clear_cache((char *)*inst_pointer, (char *)*inst_pointer + code_length);
    }

    exec_write_end();
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

static inline void grow_buffer()
{
    size_t old_size = max_code_length;
    max_code_length = old_size * 2 > old_size + JUMP_TABLE_SIZE ? old_size * 2 : old_size + JUMP_TABLE_SIZE;
    unsigned char *new_buffer = (unsigned char *)realloc_exec(*inst_pointer, old_size, max_code_length);
    if (!new_buffer)
    {
        g_core->log_error(std::format("[Dynarec] FATAL: code buffer growth failed (code_length={}, max_code_length={})",
            code_length, max_code_length));
        abort();
    }
    *inst_pointer = new_buffer;
}

void put8(unsigned char octet)
{
    if (code_length == max_code_length)
    {
        grow_buffer();
    }
    (*inst_pointer)[code_length] = octet;
    code_length++;
}

void put16(uint16_t word)
{
    if ((code_length + 2) >= max_code_length)
    {
        grow_buffer();
    }
    std::memcpy(&(*inst_pointer)[code_length], &word, sizeof(word));
    code_length += 2;
}

void put32(uint32_t dword)
{
    if ((code_length + 4) >= max_code_length)
    {
        grow_buffer();
    }
    std::memcpy(&(*inst_pointer)[code_length], &dword, sizeof(dword));
    code_length += 4;
}

void put64(uint64_t qword)
{
    if ((code_length + 8) >= max_code_length)
    {
        grow_buffer();
    }
    std::memcpy(&(*inst_pointer)[code_length], &qword, sizeof(qword));
    code_length += 8;
}

void put_insn(uint32_t insn)
{
    put32(insn);
}

void emit_mov_reg_imm64(int32_t reg, uint64_t imm)
{
    bool started = false;

    for (uint32_t shift = 0; shift < 64; shift += 16)
    {
        const uint32_t half = (uint32_t)((imm >> shift) & 0xFFFF);
        if (half == 0 && started) continue;

        if (!started)
        {
            put_insn(0xD2800000u | (shift / 16) << 21 | half << 5 | (uint32_t)reg);
            started = true;
        }
        else
        {
            put_insn(0xF2800000u | (shift / 16) << 21 | half << 5 | (uint32_t)reg);
        }
    }

    if (!started) put_insn(0xD2800000u | (uint32_t)reg);
}

void emit_ldr_x(int32_t rt, int32_t rn)
{
    put_insn(0xF9400000u | (uint32_t)rn << 5 | (uint32_t)rt);
}

void emit_str_x(int32_t rt, int32_t rn)
{
    put_insn(0xF9000000u | (uint32_t)rn << 5 | (uint32_t)rt);
}

void emit_str_w(int32_t rt, int32_t rn)
{
    put_insn(0xB9000000u | (uint32_t)rn << 5 | (uint32_t)rt);
}

void emit_blr(int32_t rn)
{
    put_insn(0xD63F0000u | (uint32_t)rn << 5);
}

void emit_br(int32_t rn)
{
    put_insn(0xD61F0000u | (uint32_t)rn << 5);
}

void emit_ret()
{
    put_insn(0xD65F03C0u);
}

int32_t emit_cbz_x(int32_t rt, int32_t insn_count)
{
    int32_t at = code_length;
    put_insn(0xB4000000u | ((uint32_t)insn_count & 0x7FFFF) << 5 | (uint32_t)rt);
    return at;
}

void patch_cbz_x(int32_t insn_offset, int32_t insn_count)
{
    uint32_t insn;
    std::memcpy(&insn, &(*inst_pointer)[insn_offset], sizeof(insn));
    insn = (insn & ~(0x7FFFFu << 5)) | (((uint32_t)insn_count & 0x7FFFF) << 5);
    std::memcpy(&(*inst_pointer)[insn_offset], &insn, sizeof(insn));
}

void mov_m32_imm32(void *m32, uint32_t imm)
{
    emit_mov_reg_imm64(A64_IP1, imm);
    emit_mov_reg_imm64(A64_IP0, (uintptr_t)m32);
    emit_str_w(A64_IP1, A64_IP0);
}

void mov_m64_imm64(void *m64, uint64_t imm)
{
    emit_mov_reg_imm64(A64_IP1, imm);
    emit_mov_reg_imm64(A64_IP0, (uintptr_t)m64);
    emit_str_x(A64_IP1, A64_IP0);
}

void jmp(uint32_t mi_addr)
{
    add_jump(code_length, mi_addr);
    put_insn(0x14000000u);
}

void debug()
{
}

void passe2(precomp_instr *dest, int32_t start, int32_t end, precomp_block *block)
{
    build_wrappers(dest, start, end, block);

    int32_t real_code_length = code_length;

    for (int32_t i = 0; i < jumps_number; i++)
    {
        const uint32_t site = jumps_table[i].pc_addr;
        precomp_instr *target_instr = &dest[(jumps_table[i].mi_addr - dest[0].addr) / 4];

        if (target_instr->reg_cache_infos.need_map)
        {
            const int64_t disp = (int64_t)real_code_length - (int64_t)site;
            const uint32_t insn = 0x14000000u | ((uint32_t)(disp >> 2) & 0x03FFFFFFu);
            std::memcpy(&(*inst_pointer)[site], &insn, sizeof(insn));

            code_length = real_code_length;
            emit_mov_reg_imm64(A64_IP0, (uintptr_t)target_instr->reg_cache_infos.jump_wrapper);
            emit_br(A64_IP0);
            real_code_length = code_length;
            continue;
        }

        const int64_t target = (int64_t)target_instr->local_addr;
        const int64_t disp = target - (int64_t)site;

        if (disp % 4 != 0 || disp < -(1 << 27) || disp >= (1 << 27))
        {
            g_core->log_error(std::format("[Dynarec] FATAL: branch displacement out of range ({})", disp));
            abort();
        }

        const uint32_t insn = 0x14000000u | ((uint32_t)(disp >> 2) & 0x03FFFFFFu);
        std::memcpy(&(*inst_pointer)[site], &insn, sizeof(insn));
    }

    code_length = real_code_length;
}

uint32_t reg_offset(const void *reg_ptr)
{
    const uintptr_t off = (uintptr_t)reg_ptr - (uintptr_t)&reg[0];
    if (off >= sizeof(reg) || (off & 7) != 0)
    {
        g_core->log_error(std::format("[Dynarec] FATAL: operand {} is not a reg[] slot", reg_ptr));
        abort();
    }
    return (uint32_t)off;
}

void emit_ldr_x_off(int32_t rt, int32_t rn, uint32_t byte_off)
{
    put_insn(0xF9400000u | ((byte_off / 8) & 0xFFF) << 10 | (uint32_t)rn << 5 | (uint32_t)rt);
}

void emit_ldr_w_off(int32_t rt, int32_t rn, uint32_t byte_off)
{
    put_insn(0xB9400000u | ((byte_off / 4) & 0xFFF) << 10 | (uint32_t)rn << 5 | (uint32_t)rt);
}

void emit_ldrsw_off(int32_t rt, int32_t rn, uint32_t byte_off)
{
    put_insn(0xB9800000u | ((byte_off / 4) & 0xFFF) << 10 | (uint32_t)rn << 5 | (uint32_t)rt);
}

void emit_str_x_off(int32_t rt, int32_t rn, uint32_t byte_off)
{
    put_insn(0xF9000000u | ((byte_off / 8) & 0xFFF) << 10 | (uint32_t)rn << 5 | (uint32_t)rt);
}

void emit_sxtw(int32_t rd, int32_t rn)
{
    put_insn(0x93407C00u | (uint32_t)rn << 5 | (uint32_t)rd);
}

static uint32_t sf_bit(bool is64)
{
    return is64 ? 0x80000000u : 0u;
}

void emit_mov_reg(int32_t rd, int32_t rm, bool is64)
{
    put_insn(0x2A0003E0u | sf_bit(is64) | (uint32_t)rm << 16 | (uint32_t)rd);
}

void emit_add(int32_t rd, int32_t rn, int32_t rm, bool is64)
{
    put_insn(0x0B000000u | sf_bit(is64) | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_sub(int32_t rd, int32_t rn, int32_t rm, bool is64)
{
    put_insn(0x4B000000u | sf_bit(is64) | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_and(int32_t rd, int32_t rn, int32_t rm, bool is64)
{
    put_insn(0x0A000000u | sf_bit(is64) | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_orr(int32_t rd, int32_t rn, int32_t rm, bool is64)
{
    put_insn(0x2A000000u | sf_bit(is64) | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_eor(int32_t rd, int32_t rn, int32_t rm, bool is64)
{
    put_insn(0x4A000000u | sf_bit(is64) | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_mvn(int32_t rd, int32_t rm, bool is64)
{
    put_insn(0x2A2003E0u | sf_bit(is64) | (uint32_t)rm << 16 | (uint32_t)rd);
}

void emit_lsl_imm_w(int32_t rd, int32_t rn, uint32_t shift)
{
    const uint32_t immr = (32u - (shift & 31)) & 31;
    const uint32_t imms = 31u - (shift & 31);
    put_insn(0x53000000u | immr << 16 | imms << 10 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_lsr_imm_w(int32_t rd, int32_t rn, uint32_t shift)
{
    put_insn(0x53000000u | (shift & 31) << 16 | 31u << 10 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_asr_imm_w(int32_t rd, int32_t rn, uint32_t shift)
{
    put_insn(0x13000000u | (shift & 31) << 16 | 31u << 10 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_lslv_w(int32_t rd, int32_t rn, int32_t rm)
{
    put_insn(0x1AC02000u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_lsrv_w(int32_t rd, int32_t rn, int32_t rm)
{
    put_insn(0x1AC02400u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_asrv_w(int32_t rd, int32_t rn, int32_t rm)
{
    put_insn(0x1AC02800u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_set_lt(int32_t rd, int32_t rn, int32_t rm, bool is_signed)
{
    put_insn(0xEB00001Fu | (uint32_t)rm << 16 | (uint32_t)rn << 5);

    const uint32_t inv_cond = is_signed ? 0xAu : 0x2u;
    put_insn(0x9A9F07E0u | inv_cond << 12 | (uint32_t)rd);
}

void emit_ldr_w_reg(int32_t rt, int32_t rn, int32_t rm)
{
    put_insn(0xB8606800u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rt);
}

void emit_ldrb_w_reg(int32_t rt, int32_t rn, int32_t rm)
{
    put_insn(0x38606800u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rt);
}

void emit_ldrh_w_reg(int32_t rt, int32_t rn, int32_t rm)
{
    put_insn(0x78606800u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rt);
}

void emit_sxtb(int32_t rd, int32_t rn)
{
    put_insn(0x93401C00u | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_sxth(int32_t rd, int32_t rn)
{
    put_insn(0x93403C00u | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_cmp_w(int32_t rn, int32_t rm)
{
    put_insn(0x6B00001Fu | (uint32_t)rm << 16 | (uint32_t)rn << 5);
}

int32_t emit_b_cond(uint32_t cond, int32_t insn_count)
{
    const int32_t at = code_length;
    put_insn(0x54000000u | ((uint32_t)insn_count & 0x7FFFF) << 5 | cond);
    return at;
}

int32_t emit_b(int32_t insn_count)
{
    const int32_t at = code_length;
    put_insn(0x14000000u | ((uint32_t)insn_count & 0x03FFFFFF));
    return at;
}

void patch_branch(int32_t insn_offset, int32_t insn_count)
{
    uint32_t insn;
    std::memcpy(&insn, &(*inst_pointer)[insn_offset], sizeof(insn));

    if ((insn & 0xFC000000u) == 0x14000000u)
    {
        insn = (insn & 0xFC000000u) | ((uint32_t)insn_count & 0x03FFFFFF);
    }
    else if ((insn & 0x7E000000u) == 0x36000000u)
    {
        insn = (insn & ~(0x3FFFu << 5)) | (((uint32_t)insn_count & 0x3FFF) << 5);
    }
    else
    {
        insn = (insn & ~(0x7FFFFu << 5)) | (((uint32_t)insn_count & 0x7FFFF) << 5);
    }

    std::memcpy(&(*inst_pointer)[insn_offset], &insn, sizeof(insn));
}

void emit_str_w_off(int32_t rt, int32_t rn, uint32_t byte_off)
{
    put_insn(0xB9000000u | ((byte_off / 4) & 0xFFF) << 10 | (uint32_t)rn << 5 | (uint32_t)rt);
}

void emit_str_w_reg(int32_t rt, int32_t rn, int32_t rm)
{
    put_insn(0xB8206800u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rt);
}

void emit_strb_w_reg(int32_t rt, int32_t rn, int32_t rm)
{
    put_insn(0x38206800u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rt);
}

void emit_strh_w_reg(int32_t rt, int32_t rn, int32_t rm)
{
    put_insn(0x78206800u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rt);
}

void emit_mul_x(int32_t rd, int32_t rn, int32_t rm)
{
    put_insn(0x9B007C00u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_umull(int32_t rd, int32_t rn, int32_t rm)
{
    put_insn(0x9BA07C00u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_asr_imm_x(int32_t rd, int32_t rn, uint32_t shift)
{
    put_insn(0x9340FC00u | (shift & 63) << 16 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_sdiv_w(int32_t rd, int32_t rn, int32_t rm)
{
    put_insn(0x1AC00C00u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_udiv_w(int32_t rd, int32_t rn, int32_t rm)
{
    put_insn(0x1AC00800u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_msub_w(int32_t rd, int32_t rn, int32_t rm, int32_t ra)
{
    put_insn(0x1B008000u | (uint32_t)rm << 16 | (uint32_t)ra << 10 | (uint32_t)rn << 5 | (uint32_t)rd);
}

int32_t emit_cbz_w(int32_t rt, int32_t insn_count)
{
    const int32_t at = code_length;
    put_insn(0x34000000u | ((uint32_t)insn_count & 0x7FFFF) << 5 | (uint32_t)rt);
    return at;
}

int32_t emit_tbz_w(int32_t rt, uint32_t bit, int32_t insn_count)
{
    const int32_t at = code_length;
    put_insn(0x36000000u | (bit & 31) << 19 | ((uint32_t)insn_count & 0x3FFF) << 5 | (uint32_t)rt);
    return at;
}

void emit_fldr(int32_t vt, int32_t rn, bool is_double)
{
    put_insn((is_double ? 0xFD400000u : 0xBD400000u) | (uint32_t)rn << 5 | (uint32_t)vt);
}

void emit_fstr(int32_t vt, int32_t rn, bool is_double)
{
    put_insn((is_double ? 0xFD000000u : 0xBD000000u) | (uint32_t)rn << 5 | (uint32_t)vt);
}

static uint32_t ftype(bool is_double)
{
    return is_double ? 0x00400000u : 0u;
}

void emit_fadd(int32_t vd, int32_t vn, int32_t vm, bool is_double)
{
    put_insn(0x1E202800u | ftype(is_double) | (uint32_t)vm << 16 | (uint32_t)vn << 5 | (uint32_t)vd);
}

void emit_fsub(int32_t vd, int32_t vn, int32_t vm, bool is_double)
{
    put_insn(0x1E203800u | ftype(is_double) | (uint32_t)vm << 16 | (uint32_t)vn << 5 | (uint32_t)vd);
}

void emit_fmul(int32_t vd, int32_t vn, int32_t vm, bool is_double)
{
    put_insn(0x1E200800u | ftype(is_double) | (uint32_t)vm << 16 | (uint32_t)vn << 5 | (uint32_t)vd);
}

void emit_fdiv(int32_t vd, int32_t vn, int32_t vm, bool is_double)
{
    put_insn(0x1E201800u | ftype(is_double) | (uint32_t)vm << 16 | (uint32_t)vn << 5 | (uint32_t)vd);
}

void emit_cmp_x(int32_t rn, int32_t rm)
{
    put_insn(0xEB00001Fu | (uint32_t)rm << 16 | (uint32_t)rn << 5);
}

void emit_cmp_x_zero(int32_t rn)
{
    put_insn(0xF100001Fu | (uint32_t)rn << 5);
}

void emit_cset(int32_t rd, uint32_t cond)
{
    put_insn(0x9A9F07E0u | (cond ^ 1u) << 12 | (uint32_t)rd);
}

void emit_lsl_imm_x(int32_t rd, int32_t rn, uint32_t shift)
{
    const uint32_t immr = (64u - (shift & 63)) & 63;
    const uint32_t imms = 63u - (shift & 63);
    put_insn(0xD3400000u | immr << 16 | imms << 10 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_lsr_imm_x(int32_t rd, int32_t rn, uint32_t shift)
{
    put_insn(0xD3400000u | (shift & 63) << 16 | 63u << 10 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_lslv_x(int32_t rd, int32_t rn, int32_t rm)
{
    put_insn(0x9AC02000u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_lsrv_x(int32_t rd, int32_t rn, int32_t rm)
{
    put_insn(0x9AC02400u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_asrv_x(int32_t rd, int32_t rn, int32_t rm)
{
    put_insn(0x9AC02800u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_fabs(int32_t vd, int32_t vn, bool is_double)
{
    put_insn(0x1E20C000u | ftype(is_double) | (uint32_t)vn << 5 | (uint32_t)vd);
}

void emit_fneg(int32_t vd, int32_t vn, bool is_double)
{
    put_insn(0x1E214000u | ftype(is_double) | (uint32_t)vn << 5 | (uint32_t)vd);
}

void emit_fsqrt(int32_t vd, int32_t vn, bool is_double)
{
    put_insn(0x1E21C000u | ftype(is_double) | (uint32_t)vn << 5 | (uint32_t)vd);
}

void emit_fmov(int32_t vd, int32_t vn, bool is_double)
{
    put_insn(0x1E204000u | ftype(is_double) | (uint32_t)vn << 5 | (uint32_t)vd);
}

void emit_fcmp(int32_t vn, int32_t vm, bool is_double)
{
    put_insn(0x1E202000u | ftype(is_double) | (uint32_t)vm << 16 | (uint32_t)vn << 5);
}

void emit_fcvt_s_to_d(int32_t vd, int32_t vn)
{
    put_insn(0x1E22C000u | (uint32_t)vn << 5 | (uint32_t)vd);
}

void emit_fcvt_d_to_s(int32_t vd, int32_t vn)
{
    put_insn(0x1E624000u | (uint32_t)vn << 5 | (uint32_t)vd);
}

static void emit_fcvt_raw(int32_t rd, int32_t vn, uint32_t rmode, bool src_double, bool dst_64)
{
    put_insn(0x1E200000u | (dst_64 ? 0x80000000u : 0u) | ftype(src_double) | (rmode & 3) << 19 | (uint32_t)vn << 5 |
             (uint32_t)rd);
}

static void emit_csel(int32_t rd, int32_t rn, int32_t rm, uint32_t cond, bool is64)
{
    put_insn((is64 ? 0x9A800000u : 0x1A800000u) | (uint32_t)rm << 16 | cond << 12 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_fcvt_to_int(int32_t rd, int32_t vn, uint32_t rmode, bool src_double, bool dst_64)
{
    if (dst_64)
    {
        emit_fcvt_raw(rd, vn, rmode, src_double, true);
        emit_mov_reg_imm64(A64_T3, 0x8000000000000000ull);
        emit_fcmp(vn, vn, src_double);
        emit_csel(rd, A64_T3, rd, A64_COND_VS, true);
        put_insn(0xB100041Fu | (uint32_t)rd << 5);
        emit_csel(rd, A64_T3, rd, A64_COND_VS, true);
        return;
    }

    emit_fcvt_raw(A64_T3, vn, rmode, src_double, true);
    emit_sxtw(rd, A64_T3);
    emit_cmp_x(rd, A64_T3);
    emit_mov_reg_imm64(A64_T3, 0x80000000u);
    emit_csel(rd, A64_T3, rd, A64_COND_NE, false);
    emit_fcmp(vn, vn, src_double);
    emit_csel(rd, A64_T3, rd, A64_COND_VS, false);
}

void emit_scvtf(int32_t vd, int32_t rn, bool dst_double, bool src_64)
{
    put_insn(0x1E220000u | (src_64 ? 0x80000000u : 0u) | ftype(dst_double) | (uint32_t)rn << 5 | (uint32_t)vd);
}

void emit_frintx(int32_t vd, int32_t vn, bool is_double)
{
    put_insn(0x1E274000u | ftype(is_double) | (uint32_t)vn << 5 | (uint32_t)vd);
}

void emit_umulh(int32_t rd, int32_t rn, int32_t rm)
{
    put_insn(0x9BC07C00u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_add_imm_x(int32_t rd, int32_t rn, uint32_t imm12)
{
    put_insn(0x91000000u | (imm12 & 0xFFF) << 10 | (uint32_t)rn << 5 | (uint32_t)rd);
}

void emit_sub_imm_x(int32_t rd, int32_t rn, uint32_t imm12)
{
    put_insn(0xD1000000u | (imm12 & 0xFFF) << 10 | (uint32_t)rn << 5 | (uint32_t)rd);
}
