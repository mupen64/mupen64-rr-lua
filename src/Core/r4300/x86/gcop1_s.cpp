/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Core.h>
#include <r4300/r4300.h>
#include <r4300/recomph.h>
#include <r4300/x86/assemble.h>
#include <r4300/x86/gcop1_helpers.h>

static void gencheck_eax_valid(int32_t stackBase)
{
    if (!g_core->cfg->float_exception_emulation)
        return;

    mov_reg32_imm32(EBX, (uint32_t)&largest_denormal_float);
    fld_preg32_dword(EBX);
    fld_preg32_dword(EAX);
    gencheck_float_input_valid(stackBase);
}

static void gencheck_result_valid()
{
    if (!g_core->cfg->float_exception_emulation)
        return;

    mov_reg32_imm32(EBX, (uint32_t)&largest_denormal_float);
    fld_preg32_dword(EBX);
    gencheck_float_output_valid();
}

void genadd_s()
{
#ifdef INTERPRET_ADD_S
    gencallinterp((uint32_t)ADD_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.ft]));
    gencheck_eax_valid(1);
    fadd_preg32_dword(EAX);
    gencheck_result_valid();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    fstp_preg32_dword(EAX);
#endif
}

void gensub_s()
{
#ifdef INTERPRET_SUB_S
    gencallinterp((uint32_t)SUB_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.ft]));
    gencheck_eax_valid(1);
    fsub_preg32_dword(EAX);
    gencheck_result_valid();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    fstp_preg32_dword(EAX);
#endif
}

void genmul_s()
{
#ifdef INTERPRET_MUL_S
    gencallinterp((uint32_t)MUL_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.ft]));
    gencheck_eax_valid(1);
    fmul_preg32_dword(EAX);
    gencheck_result_valid();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    fstp_preg32_dword(EAX);
#endif
}

void gendiv_s()
{
#ifdef INTERPRET_DIV_S
    gencallinterp((uint32_t)DIV_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.ft]));
    gencheck_eax_valid(1);
    fdiv_preg32_dword(EAX);
    gencheck_result_valid();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    fstp_preg32_dword(EAX);
#endif
}

void gensqrt_s()
{
#ifdef INTERPRET_SQRT_S
    gencallinterp((uint32_t)SQRT_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fld_preg32_dword(EAX);
    fsqrt();
    gencheck_result_valid();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    fstp_preg32_dword(EAX);
#endif
}

void genabs_s()
{
#ifdef INTERPRET_ABS_S
    gencallinterp((uint32_t)ABS_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_eax_valid(1);
    fld_preg32_dword(EAX);
    fabs_();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    fstp_preg32_dword(EAX);
#endif
}

void genmov_s()
{
#ifdef INTERPRET_MOV_S
    gencallinterp((uint32_t)MOV_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    mov_reg32_preg32(EBX, EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    mov_preg32_reg32(EAX, EBX);
#endif
}

void genneg_s()
{
#ifdef INTERPRET_NEG_S
    gencallinterp((uint32_t)NEG_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_dword(EAX);
    fchs();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    fstp_preg32_dword(EAX);
#endif
}

void genround_l_s()
{
#ifdef INTERPRET_ROUND_L_S
    gencallinterp((uint32_t)ROUND_L_S, 0);
#else
    gencheck_cop1_unusable();
    fldcw_m16((uint16_t*)&round_mode);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    fistp_preg32_qword(EAX);
    fldcw_m16((uint16_t*)&rounding_mode);
    gencheck_float_conversion_valid();
#endif
}

void gentrunc_l_s()
{
#ifdef INTERPRET_TRUNC_L_S
    gencallinterp((uint32_t)TRUNC_L_S, 0);
#else
    gencheck_cop1_unusable();
    fldcw_m16((uint16_t*)&trunc_mode);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    fistp_preg32_qword(EAX);
    fldcw_m16((uint16_t*)&rounding_mode);
    gencheck_float_conversion_valid();
#endif
}

void genceil_l_s()
{
#ifdef INTERPRET_CEIL_L_S
    gencallinterp((uint32_t)CEIL_L_S, 0);
#else
    gencheck_cop1_unusable();
    fldcw_m16((uint16_t*)&ceil_mode);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    fistp_preg32_qword(EAX);
    fldcw_m16((uint16_t*)&rounding_mode);
    gencheck_float_conversion_valid();
#endif
}

void genfloor_l_s()
{
#ifdef INTERPRET_FLOOR_L_S
    gencallinterp((uint32_t)FLOOR_L_S, 0);
#else
    gencheck_cop1_unusable();
    fldcw_m16((uint16_t*)&floor_mode);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    fistp_preg32_qword(EAX);
    fldcw_m16((uint16_t*)&rounding_mode);
    gencheck_float_conversion_valid();
#endif
}

void genround_w_s()
{
#ifdef INTERPRET_ROUND_W_S
    gencallinterp((uint32_t)ROUND_W_S, 0);
#else
    gencheck_cop1_unusable();
    fldcw_m16((uint16_t*)&round_mode);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    fistp_preg32_dword(EAX);
    fldcw_m16((uint16_t*)&rounding_mode);
    gencheck_float_conversion_valid();
#endif
}

void gentrunc_w_s()
{
#ifdef INTERPRET_TRUNC_W_S
    gencallinterp((uint32_t)TRUNC_W_S, 0);
#else
    gencheck_cop1_unusable();
    fldcw_m16((uint16_t*)&trunc_mode);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    fistp_preg32_dword(EAX);
    fldcw_m16((uint16_t*)&rounding_mode);
    gencheck_float_conversion_valid();
#endif
}

void genceil_w_s()
{
#ifdef INTERPRET_CEIL_W_S
    gencallinterp((uint32_t)CEIL_W_S, 0);
#else
    gencheck_cop1_unusable();
    fldcw_m16((uint16_t*)&ceil_mode);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    fistp_preg32_dword(EAX);
    fldcw_m16((uint16_t*)&rounding_mode);
    gencheck_float_conversion_valid();
#endif
}

void genfloor_w_s()
{
#ifdef INTERPRET_FLOOR_W_S
    gencallinterp((uint32_t)FLOOR_W_S, 0);
#else
    gencheck_cop1_unusable();
    fldcw_m16((uint16_t*)&floor_mode);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    fistp_preg32_dword(EAX);
    fldcw_m16((uint16_t*)&rounding_mode);
    gencheck_float_conversion_valid();
#endif
}

void gencvt_d_s()
{
#ifdef INTERPRET_CVT_D_S
    gencallinterp((uint32_t)CVT_D_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    fstp_preg32_qword(EAX);
#endif
}

void gencvt_w_s()
{
#ifdef INTERPRET_CVT_W_S
    gencallinterp((uint32_t)CVT_W_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    fistp_preg32_dword(EAX);
    gencheck_float_conversion_valid();
#endif
}

void gencvt_l_s()
{
#ifdef INTERPRET_CVT_L_S
    gencallinterp((uint32_t)CVT_L_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    fistp_preg32_qword(EAX);
    gencheck_float_conversion_valid();
#endif
}

void genc_f_s()
{
#ifdef INTERPRET_C_F_S
    gencallinterp((uint32_t)C_F_S, 0);
#else
    gencheck_cop1_unusable();
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000);
#endif
}

void genc_un_s()
{
#ifdef INTERPRET_C_UN_S
    gencallinterp((uint32_t)C_UN_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.ft]));
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    fld_preg32_dword(EAX);
    fucomip_fpreg(1);
    ffree_fpreg(0);
    jp_rj(12);
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
    jmp_imm_short(10); // 2
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
#endif
}

void genc_eq_s()
{
#ifdef INTERPRET_C_EQ_S
    gencallinterp((uint32_t)C_EQ_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.ft]));
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    fld_preg32_dword(EAX);
    fucomip_fpreg(1);
    ffree_fpreg(0);
    jne_rj(12);
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_ueq_s()
{
#ifdef INTERPRET_C_UEQ_S
    gencallinterp((uint32_t)C_UEQ_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.ft]));
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    fld_preg32_dword(EAX);
    fucomip_fpreg(1);
    ffree_fpreg(0);
    jp_rj(14);
    jne_rj(12);
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_olt_s()
{
#ifdef INTERPRET_C_OLT_S
    gencallinterp((uint32_t)C_OLT_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.ft]));
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    fld_preg32_dword(EAX);
    fucomip_fpreg(1);
    ffree_fpreg(0);
    jae_rj(12);
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_ult_s()
{
#ifdef INTERPRET_C_ULT_S
    gencallinterp((uint32_t)C_ULT_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.ft]));
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    fld_preg32_dword(EAX);
    fucomip_fpreg(1);
    ffree_fpreg(0);
    jp_rj(14);
    jae_rj(12);
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_ole_s()
{
#ifdef INTERPRET_C_OLE_S
    gencallinterp((uint32_t)C_OLE_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.ft]));
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    fld_preg32_dword(EAX);
    fucomip_fpreg(1);
    ffree_fpreg(0);
    ja_rj(12);
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_ule_s()
{
#ifdef INTERPRET_C_ULE_S
    gencallinterp((uint32_t)C_ULE_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.ft]));
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    fld_preg32_dword(EAX);
    fucomip_fpreg(1);
    ffree_fpreg(0);
    jp_rj(14);
    ja_rj(12);
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_sf_s()
{
#ifdef INTERPRET_C_SF_S
    gencallinterp((uint32_t)C_SF_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.ft]));
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    fld_preg32_dword(EAX);
    fcomip_fpreg(1);
    ffree_fpreg(0);
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000);
#endif
}

void genc_ngle_s()
{
#ifdef INTERPRET_C_NGLE_S
    gencallinterp((uint32_t)C_NGLE_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.ft]));
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    fld_preg32_dword(EAX);
    fcomip_fpreg(1);
    ffree_fpreg(0);
    jp_rj(12);
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
    jmp_imm_short(10); // 2
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
#endif
}

void genc_seq_s()
{
#ifdef INTERPRET_C_SEQ_S
    gencallinterp((uint32_t)C_SEQ_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.ft]));
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    fld_preg32_dword(EAX);
    fcomip_fpreg(1);
    ffree_fpreg(0);
    jne_rj(12);
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_ngl_s()
{
#ifdef INTERPRET_C_NGL_S
    gencallinterp((uint32_t)C_NGL_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.ft]));
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    fld_preg32_dword(EAX);
    fcomip_fpreg(1);
    ffree_fpreg(0);
    jp_rj(14);
    jne_rj(12);
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_lt_s()
{
#ifdef INTERPRET_C_LT_S
    gencallinterp((uint32_t)C_LT_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.ft]));
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    fld_preg32_dword(EAX);
    fcomip_fpreg(1);
    ffree_fpreg(0);
    jae_rj(12);
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_nge_s()
{
#ifdef INTERPRET_C_NGE_S
    gencallinterp((uint32_t)C_NGE_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.ft]));
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    fld_preg32_dword(EAX);
    fcomip_fpreg(1);
    ffree_fpreg(0);
    jp_rj(14);
    jae_rj(12);
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_le_s()
{
#ifdef INTERPRET_C_LE_S
    gencallinterp((uint32_t)C_LE_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.ft]));
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    fld_preg32_dword(EAX);
    fcomip_fpreg(1);
    ffree_fpreg(0);
    ja_rj(12);
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_ngt_s()
{
#ifdef INTERPRET_C_NGT_S
    gencallinterp((uint32_t)C_NGT_S, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.ft]));
    fld_preg32_dword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fs]));
    fld_preg32_dword(EAX);
    fcomip_fpreg(1);
    ffree_fpreg(0);
    jp_rj(14);
    ja_rj(12);
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}
