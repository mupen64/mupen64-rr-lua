/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <cstdint>

#define A64_X0 0
#define A64_X1 1
#define A64_IP0 16
#define A64_IP1 17
#define A64_LR 30
#define A64_ZR 31
#define A64_SP 31

#define A64_T0 9
#define A64_T1 10
#define A64_T2 11
#define A64_T3 13

#define A64_RB 19

typedef struct
{
    int32_t need_map;

    void *needed_registers[8];

    unsigned char *jump_wrapper;
} reg_cache_struct;

extern int32_t branch_taken;

void debug();

void put8(unsigned char octet);
void put16(uint16_t word);
void put32(uint32_t dword);
void put64(uint64_t qword);

void put_insn(uint32_t insn);

void emit_mov_reg_imm64(int32_t reg, uint64_t imm);

void emit_ldr_x(int32_t rt, int32_t rn);
void emit_str_x(int32_t rt, int32_t rn);
void emit_str_w(int32_t rt, int32_t rn);
void emit_blr(int32_t rn);
void emit_br(int32_t rn);
void emit_ret();

int32_t emit_cbz_x(int32_t rt, int32_t insn_count);
void patch_cbz_x(int32_t insn_offset, int32_t insn_count);

void mov_m32_imm32(void *m32, uint32_t imm);
void mov_m64_imm64(void *m64, uint64_t imm);

void jmp(uint32_t mi_addr);

uint32_t reg_offset(const void *reg_ptr);

inline void emit_reg_base()
{
}

void emit_ldr_x_off(int32_t rt, int32_t rn, uint32_t byte_off);
void emit_ldr_w_off(int32_t rt, int32_t rn, uint32_t byte_off);
void emit_ldrsw_off(int32_t rt, int32_t rn, uint32_t byte_off);
void emit_str_x_off(int32_t rt, int32_t rn, uint32_t byte_off);
void emit_str_w_off(int32_t rt, int32_t rn, uint32_t byte_off);

void emit_sxtw(int32_t rd, int32_t rn);

void emit_add_imm_x(int32_t rd, int32_t rn, uint32_t imm12);
void emit_sub_imm_x(int32_t rd, int32_t rn, uint32_t imm12);
void emit_mov_reg(int32_t rd, int32_t rm, bool is64);

void emit_add(int32_t rd, int32_t rn, int32_t rm, bool is64);
void emit_sub(int32_t rd, int32_t rn, int32_t rm, bool is64);
void emit_and(int32_t rd, int32_t rn, int32_t rm, bool is64);
void emit_orr(int32_t rd, int32_t rn, int32_t rm, bool is64);
void emit_eor(int32_t rd, int32_t rn, int32_t rm, bool is64);
void emit_mvn(int32_t rd, int32_t rm, bool is64);

void emit_lsl_imm_x(int32_t rd, int32_t rn, uint32_t shift);
void emit_lsr_imm_x(int32_t rd, int32_t rn, uint32_t shift);

void emit_lslv_x(int32_t rd, int32_t rn, int32_t rm);
void emit_lsrv_x(int32_t rd, int32_t rn, int32_t rm);
void emit_asrv_x(int32_t rd, int32_t rn, int32_t rm);

void emit_lsl_imm_w(int32_t rd, int32_t rn, uint32_t shift);
void emit_lsr_imm_w(int32_t rd, int32_t rn, uint32_t shift);
void emit_asr_imm_w(int32_t rd, int32_t rn, uint32_t shift);

void emit_lslv_w(int32_t rd, int32_t rn, int32_t rm);
void emit_lsrv_w(int32_t rd, int32_t rn, int32_t rm);
void emit_asrv_w(int32_t rd, int32_t rn, int32_t rm);

void emit_set_lt(int32_t rd, int32_t rn, int32_t rm, bool is_signed);

void emit_ldr_w_reg(int32_t rt, int32_t rn, int32_t rm);
void emit_ldrb_w_reg(int32_t rt, int32_t rn, int32_t rm);
void emit_ldrh_w_reg(int32_t rt, int32_t rn, int32_t rm);

void emit_str_w_reg(int32_t rt, int32_t rn, int32_t rm);
void emit_strb_w_reg(int32_t rt, int32_t rn, int32_t rm);
void emit_strh_w_reg(int32_t rt, int32_t rn, int32_t rm);

void emit_sxtb(int32_t rd, int32_t rn);
void emit_sxth(int32_t rd, int32_t rn);

void emit_cmp_w(int32_t rn, int32_t rm);
void emit_cmp_x(int32_t rn, int32_t rm);
void emit_cmp_x_zero(int32_t rn);

void emit_cset(int32_t rd, uint32_t cond);

void gen_test_rs_rt(uint32_t cond);
void gen_test_rs_zero(uint32_t cond);

void gen_dispatch_pending();

void gentestl_impl(bool count_on_fallthrough);
void gentestl_out_impl(bool count_on_fallthrough);

void emit_mul_x(int32_t rd, int32_t rn, int32_t rm);
void emit_umull(int32_t rd, int32_t rn, int32_t rm);
void emit_umulh(int32_t rd, int32_t rn, int32_t rm);
void emit_asr_imm_x(int32_t rd, int32_t rn, uint32_t shift);
void emit_sdiv_w(int32_t rd, int32_t rn, int32_t rm);
void emit_udiv_w(int32_t rd, int32_t rn, int32_t rm);
void emit_msub_w(int32_t rd, int32_t rn, int32_t rm, int32_t ra);
int32_t emit_cbz_w(int32_t rt, int32_t insn_count);

int32_t emit_tbz_w(int32_t rt, uint32_t bit, int32_t insn_count);

void emit_fldr(int32_t vt, int32_t rn, bool is_double);
void emit_fstr(int32_t vt, int32_t rn, bool is_double);
void emit_fadd(int32_t vd, int32_t vn, int32_t vm, bool is_double);
void emit_fsub(int32_t vd, int32_t vn, int32_t vm, bool is_double);
void emit_fmul(int32_t vd, int32_t vn, int32_t vm, bool is_double);
void emit_fdiv(int32_t vd, int32_t vn, int32_t vm, bool is_double);
void emit_fabs(int32_t vd, int32_t vn, bool is_double);
void emit_fneg(int32_t vd, int32_t vn, bool is_double);
void emit_fsqrt(int32_t vd, int32_t vn, bool is_double);
void emit_fmov(int32_t vd, int32_t vn, bool is_double);
void emit_fcmp(int32_t vn, int32_t vm, bool is_double);

#define A64_RMODE_NEAREST 0
#define A64_RMODE_PLUS_INF 1
#define A64_RMODE_MINUS_INF 2
#define A64_RMODE_ZERO 3
void emit_fcvt_to_int(int32_t rd, int32_t vn, uint32_t rmode, bool src_double, bool dst_64);

void emit_scvtf(int32_t vd, int32_t rn, bool dst_double, bool src_64);

void emit_frintx(int32_t vd, int32_t vn, bool is_double);

void emit_fcvt_s_to_d(int32_t vd, int32_t vn);
void emit_fcvt_d_to_s(int32_t vd, int32_t vn);

#define A64_COND_EQ 0
#define A64_COND_NE 1
#define A64_COND_MI 4
#define A64_COND_VS 6
#define A64_COND_HI 8
#define A64_COND_LS 9
#define A64_COND_GE 10
#define A64_COND_LT 11
#define A64_COND_GT 12
#define A64_COND_LE 13
int32_t emit_b_cond(uint32_t cond, int32_t insn_count);
int32_t emit_b(int32_t insn_count);
void patch_branch(int32_t insn_offset, int32_t insn_count);
