/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#define EAX 0
#define ECX 1
#define EDX 2
#define EBX 3
#define ESP 4
#define EBP 5
#define ESI 6
#define EDI 7

#define AX 0
#define CX 1
#define DX 2
#define BX 3
#define SP 4
#define BP 5
#define SI 6
#define DI 7

#define AL 0
#define CL 1
#define DL 2
#define BL 3
#define AH 4
#define CH 5
#define DH 6
#define BH 7

typedef struct _reg_cache_struct
{
    int32_t need_map;
    void *needed_registers[8];
    unsigned char jump_wrapper[256];
    int32_t need_cop1_check;
} reg_cache_struct;

extern int32_t branch_taken;

void debug();

void put8(unsigned char octet);
extern void put32(uint32_t dword);
extern void put64(uint64_t qword);
void put16(uint16_t word);

void push_reg32(int32_t reg32);
void mov_eax_memoffs32(void *_memoffs32);
void cmp_reg32_m32(int32_t reg32, void *_m32);
void jne_rj(unsigned char saut);
// Patch a previously-emitted rel8 short-jump displacement (jcc/jmp) so it
// targets the current emission point. `disp_pos` is the offset of the
// displacement byte (i.e. code_length-1 right after emitting the jump with a
// placeholder of 0). This avoids hand-counting x64 instruction sizes.
void rj_patch(int32_t disp_pos);
// Patch a previously-emitted rel32 near-jump displacement (e.g. je_near_rj(0))
// so it targets the current emission point. `disp_pos` is the offset of the
// 4-byte displacement field (code_length-4 right after emitting the jump).
void rj_patch_near(int32_t disp_pos);
void mov_reg32_imm32(int32_t reg32, uint32_t imm32);
void mov_reg64_imm64(int32_t reg32, uintptr_t imm64);
void add_reg64_reg64(uint32_t reg1, uint32_t reg2);
void mov_reg32_preg64(int32_t dst, int32_t base);
void mov_reg64_preg64(int32_t dst, int32_t base);
void jmp_imm(int32_t saut);
void dec_reg32(int32_t reg32);
void mov_memoffs32_eax(void *_memoffs32);
void or_m32_imm32(void *_m32, uint32_t imm32);
void pop_reg32(int32_t reg32);
void add_m32_imm32(void *_m32, uint32_t imm32);
void inc_reg32(int32_t reg32);
void push_imm32(uint32_t imm32);
void add_reg32_imm8(uint32_t reg32, unsigned char imm8);
void inc_m32(void *_m32);
void cmp_m32_imm32(void *_m32, uint32_t imm32);
void mov_m32_imm32(void *_m32, uint32_t imm32);
void mov_m64_imm64(void *_m64, uintptr_t imm64);
void je_rj(unsigned char saut);
void jmp(uint32_t mi_addr);
void cdq();
void mov_m32_reg32(void *_m32, uint32_t reg32);
void mov_m64_reg64(void *_m64, uint32_t reg64); // x64: save full 64-bit register
void movsxd_reg64_reg32(uint32_t dst, uint32_t src); // sign-extend 32-bit reg into 64-bit reg
void mov_reg64_m64(uint32_t reg64, void *_m64); // x64: load full 64-bit register
void je_near(uint32_t mi_addr);
void jne_near(uint32_t mi_addr);
void jge_near(uint32_t mi_addr);
void jle_rj(unsigned char saut);
void jge_rj(unsigned char saut);
void ret();
void jle_near(uint32_t mi_addr);
void call_reg32(uint32_t reg32);
void call_reg64(uint32_t reg32);
void jne_near_rj(uint32_t saut);
void jl_rj(unsigned char saut);
void sub_reg32_m32(int32_t reg32, void *_m32);
void shr_reg32_imm8(uint32_t reg32, unsigned char imm8);
void mul_m32(void *_m32);
void add_reg32_reg32(uint32_t reg1, uint32_t reg2);
void add_reg32_m32(uint32_t reg32, void *_m32);
void jmp_reg32(uint32_t reg32);
void jmp_reg64(uint32_t reg64);
void sub_reg32_imm32(int32_t reg32, uint32_t imm32);
void mov_reg32_preg32(uint32_t reg1, uint32_t reg2);
void sub_eax_imm32(uint32_t imm32);
void add_reg32_imm32(uint32_t reg32, uint32_t imm32);
void jl_near(uint32_t mi_addr);
void add_eax_imm32(uint32_t imm32);
void shl_reg32_imm8(uint32_t reg32, unsigned char imm8);
void mov_reg32_m32(uint32_t reg32, void *_m32);
void and_eax_imm32(uint32_t imm32);
void mov_al_memoffs8(unsigned char *memoffs8);
void mov_memoffs8_al(unsigned char *memoffs8);
void nop();
void mov_ax_memoffs16(uint16_t *memoffs16);
void mov_memoffs16_ax(uint16_t *memoffs16);
void cwde();
void jb_rj(unsigned char saut);
void ja_rj(unsigned char saut);
void jg_rj(unsigned char saut);
void and_ax_imm16(uint16_t imm16);
void or_ax_imm16(uint16_t imm16);
void xor_ax_imm16(uint16_t imm16);
void shrd_reg32_reg32_imm8(uint32_t reg1, uint32_t reg2, unsigned char imm8);
void or_eax_imm32(uint32_t imm32);
void or_m32_reg32(void *_m32, uint32_t reg32);
void or_reg32_reg32(uint32_t reg1, uint32_t reg2);
void sar_reg32_imm8(uint32_t reg32, unsigned char imm8);
void and_reg32_reg32(uint32_t reg1, uint32_t reg2);
void xor_reg32_reg32(uint32_t reg1, uint32_t reg2);
void imul_m32(void *_m32);
void mov_reg32_reg32(uint32_t reg1, uint32_t reg2);
void idiv_m32(void *_m32);
void shr_reg32_cl(uint32_t reg32);
void shl_reg32_cl(uint32_t reg32);
void cmp_eax_imm32(uint32_t imm32);
void cmp_eax_r11(); // x64: cmp eax, r11 (compare against 64-bit value in r11)
void cmp_rax_r11(); // x64: cmp rax, r11 (full 64-bit comparison)
void mov_rax_preg32x4pimm32(int32_t reg1, int32_t reg2, uint32_t imm32); // x64: mov rax, [reg1*4 + reg2 + imm32]
void cmp_rax_imm64(uintptr_t imm64); // x64: mov r11, imm64 ; cmp rax, r11
void jg_near(uint32_t mi_addr);
void add_m32_reg32(void *_m32, int32_t reg32);
void je_near_rj(uint32_t saut);
void jge_near_rj(uint32_t saut);
void jl_near_rj(uint32_t saut);
void jle_near_rj(uint32_t saut);
void call_m32(void *_m32);
void and_reg32_m32(uint32_t reg32, void *_m32);
void or_reg32_m32(uint32_t reg32, void *_m32);
void not_reg32(uint32_t reg32);
void xor_reg32_m32(uint32_t reg32, void *_m32);
void sar_reg32_cl(uint32_t reg32);
void jmp_imm_short(char saut);
void jmp_m32(void *_m32);
void mov_reg32_preg32preg32pimm32(int32_t reg1, int32_t reg2, int32_t reg3, uint32_t imm32);
void mov_reg32_preg32pimm32(int32_t reg1, int32_t reg2, uint32_t imm32);
void cmp_m32_imm8(void *_m32, unsigned char imm8);
void mov_reg32_preg32x4preg32(int32_t reg1, int32_t reg2, int32_t reg3);
void mov_reg32_preg32x4preg32pimm32(int32_t reg1, int32_t reg2, int32_t reg3, uint32_t imm32);
void mov_reg32_preg32x4pimm32(int32_t reg1, int32_t reg2, uint32_t imm32);
void mov_reg32_preg32x4pimm64(int32_t reg1, int32_t reg2, uint64_t ptr); // x64: load 64-bit ptr into R10 then mov [idx*4+R10]
void mov_reg32_preg32x4_r10(int32_t dst_reg, int32_t index_reg, uint64_t func_ptr); // x64: load 64-bit func_ptr into R10 then mov [idx*4+R10]
void cmp_reg32_prx4_r10(int32_t index_reg, uint64_t func_ptr); // x64: load 64-bit func_ptr into R10, mov to r11, for compare ops
void mov_rax_r11_idx4(int32_t idx_reg); // x64: mov rax, [r11 + idx*4] - base already in r11
void and_reg32_imm32(int32_t reg32, uint32_t imm32);
void and_reg32_imm32(int32_t reg32, uint32_t imm32);
void movsx_reg32_m8(int32_t reg32, unsigned char *m8);
void sub_reg32_reg32(int32_t reg1, int32_t reg2);
void cbw();
void movsx_reg32_reg8(int32_t reg32, int32_t reg8);
void and_reg32_imm8(int32_t reg32, unsigned char imm8);
void movsx_reg32_reg16(int32_t reg32, int32_t reg8);
void movsx_reg32_m16(int32_t reg32, uint16_t *m16);
void cmp_reg32_imm8(int32_t reg32, unsigned char imm8);
void add_m32_imm8(void *_m32, unsigned char imm8);
void mov_reg8_m8(int32_t reg8, unsigned char *m8);
void mov_preg32preg32pimm32_reg8(int32_t reg1, int32_t reg2, uint32_t imm32, int32_t reg8);
void mov_preg32pimm32_reg16(int32_t reg32, uint32_t imm32, int32_t reg16);
void cmp_reg32_imm32(int32_t reg32, uint32_t imm32);
void mov_preg32pimm32_reg32(int32_t reg1, uint32_t imm32, int32_t reg2);
void fld_fpreg(int32_t fpreg);
void fld_preg32_dword(int32_t reg32);
void fdiv_preg32_dword(int32_t reg32);
void fstp_fpreg(int32_t fpreg);
void fstp_preg32_dword(int32_t reg32);
void mov_preg32_reg32(int32_t reg1, int32_t reg2);
void fldz();
void fchs();
void fstp_preg32_qword(int32_t reg32);
void fadd_preg32_dword(int32_t reg32);
void fmul_preg32_dword(int32_t reg32);
void fcomp_preg32_dword(int32_t reg32);
void and_m32_imm32(void *_m32, uint32_t imm32);
void fistp_m32(void *_m32);
void fistp_m64(uint64_t *m64);
void div_m32(void *_m32);
void mov_m8_imm8(unsigned char *m8, unsigned char imm8);
void mov_reg16_m16(int32_t reg16, uint16_t *m16);
void mov_m16_reg16(uint16_t *m16, int32_t reg16);
void fld_preg32_qword(int32_t reg32);
void fadd_preg32_qword(int32_t reg32);
void fdiv_preg32_qword(int32_t reg32);
void fsub_preg32_dword(int32_t reg32);
void xor_reg32_imm32(int32_t reg32, uint32_t imm32);
void xor_al_imm8(unsigned char imm8);
void mov_preg32pimm32_reg8(int32_t reg32, uint32_t imm32, int32_t reg8);
void xor_reg8_imm8(int32_t reg8, unsigned char imm8);
void cmp_m8_imm8(unsigned char *m8, unsigned char imm8);
void fsub_preg32_qword(int32_t reg32);
void fmul_preg32_qword(int32_t reg32);
void adc_reg32_m32(uint32_t reg32, void *_m32);
void sub_m32_imm32(void *_m32, uint32_t imm32);
void jbe_rj(unsigned char saut);
void cmp_reg32_reg32(int32_t reg1, int32_t reg2);
void or_reg32_imm32(int32_t reg32, uint32_t imm32);
void adc_reg32_imm32(uint32_t reg32, uint32_t imm32);
void and_al_imm8(unsigned char imm8);
void test_al_imm8(unsigned char imm8);
void cmp_al_imm8(unsigned char imm8);
void movsx_reg32_8preg32pimm32(int32_t reg1, int32_t reg2, uint32_t imm32);
void movsx_reg32_16preg32pimm32(int32_t reg1, int32_t reg2, uint32_t imm32);
void mov_preg32pimm32_imm8(int32_t reg32, uint32_t imm32, unsigned char imm8);
void mov_m8_reg8(unsigned char *m8, int32_t reg8);
void shld_reg32_reg32_cl(uint32_t reg1, uint32_t reg2);
void test_reg32_imm32(int32_t reg32, uint32_t imm32);
void shrd_reg32_reg32_cl(uint32_t reg1, uint32_t reg2);
void imul_reg32(uint32_t reg32);
void mul_reg32(uint32_t reg32);
void idiv_reg32(uint32_t reg32);
void div_reg32(uint32_t reg32);
void adc_reg32_reg32(uint32_t reg1, uint32_t reg2);
void sbb_reg32_reg32(int32_t reg1, int32_t reg2);
void shld_reg32_reg32_imm8(uint32_t reg1, uint32_t reg2, unsigned char imm8);
void cmp_preg32pimm32_imm8(int32_t reg32, uint32_t imm32, unsigned char imm8);
void test_m32_imm32(void *_m32, uint32_t imm32);
void fldcw_m16(uint16_t *m16);
void fsqrt();
void fabs_();
void fistp_preg32_qword(int32_t reg32);
void fistp_preg32_dword(int32_t reg32);
void fcomip_fpreg(int32_t fpreg);
void fucomi_fpreg(int32_t fpreg);
void fucomip_fpreg(int32_t fpreg);
void ffree_fpreg(int32_t fpreg);
void jp_rj(unsigned char saut);
void jae_rj(unsigned char saut);
void fild_preg32_qword(int32_t reg32);
void fild_preg32_dword(int32_t reg32);
void fclex();
void fstsw_ax();
void ud2();

// x64: 64-bit address variants — load addr into R11, add index reg, dereference [r11]
void cmp_preg32pimm32_imm8_r11(int32_t reg32, uintptr_t addr64, unsigned char imm8);
void mov_reg32_preg32pimm32_r11(int32_t reg1, int32_t reg2, uintptr_t addr64);
void mov_preg32pimm32_reg8_r11(int32_t reg32, uintptr_t addr64, int32_t reg8);
void mov_preg32pimm32_imm8_r11(int32_t reg32, uintptr_t addr64, unsigned char imm8);
void mov_preg32pimm32_reg16_r11(int32_t reg32, uintptr_t addr64, int32_t reg16);
void mov_preg32pimm32_reg32_r11(int32_t reg1, uintptr_t addr64, int32_t reg2);
void movsx_reg32_8preg32pimm32_r11(int32_t reg1, int32_t reg2, uintptr_t addr64);
void movsx_reg32_16preg32pimm32_r11(int32_t reg1, int32_t reg2, uintptr_t addr64);
