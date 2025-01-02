/**
 * Mupen64 - gcop1_d.c
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

#include <shared/Config.hpp>
#include "../ops.h"
#include "../r4300.h"
#include "../recomph.h"
#include "assemble.h"
#include "gcop1_helpers.h"
#include "interpret.h"

static void gencheck_eax_valid(int32_t stackBase)
{
    if (!g_config.is_float_exception_propagation_enabled)
        return;

    mov_reg32_imm32(EBX, (uint32_t)&largest_denormal_double);
    fld_preg32_qword(EBX);
    fld_preg32_qword(EAX);
    gencheck_float_input_valid(stackBase);
}

static void gencheck_result_valid()
{
    if (!g_config.is_float_exception_propagation_enabled)
        return;

    mov_reg32_imm32(EBX, (uint32_t)&largest_denormal_double);
    fld_preg32_qword(EBX);
    gencheck_float_output_valid();
}

static void gencheck_result_valid_s()
{
    if (!g_config.is_float_exception_propagation_enabled)
        return;

    mov_reg32_imm32(EBX, (uint32_t)&largest_denormal_float);
    fld_preg32_dword(EBX);
    gencheck_float_output_valid();
}

void genadd_d()
{
#ifdef INTERPRET_ADD_D
    gencallinterp((uint32_t)ADD_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.ft]));
    gencheck_eax_valid(1);
    fadd_preg32_qword(EAX);
    gencheck_result_valid();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    fstp_preg32_qword(EAX);
#endif
}

void gensub_d()
{
#ifdef INTERPRET_SUB_D
    gencallinterp((uint32_t)SUB_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.ft]));
    gencheck_eax_valid(1);
    fsub_preg32_qword(EAX);
    gencheck_result_valid();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    fstp_preg32_qword(EAX);
#endif
}

void genmul_d()
{
#ifdef INTERPRET_MUL_D
    gencallinterp((uint32_t)MUL_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.ft]));
    gencheck_eax_valid(1);
    fmul_preg32_qword(EAX);
    gencheck_result_valid();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    fstp_preg32_qword(EAX);
#endif
}

void gendiv_d()
{
#ifdef INTERPRET_DIV_D
    gencallinterp((uint32_t)DIV_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.ft]));
    gencheck_eax_valid(1);
    fdiv_preg32_qword(EAX);
    gencheck_result_valid();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    fstp_preg32_qword(EAX);
#endif
}

void gensqrt_d()
{
#ifdef INTERPRET_SQRT_D
    gencallinterp((uint32_t)SQRT_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fld_preg32_qword(EAX);
    fsqrt();
    gencheck_result_valid();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    fstp_preg32_qword(EAX);
#endif
}

void genabs_d()
{
#ifdef INTERPRET_ABS_D
    gencallinterp((uint32_t)ABS_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fld_preg32_qword(EAX);
    fabs_();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    fstp_preg32_qword(EAX);
#endif
}

void genmov_d()
{
#ifdef INTERPRET_MOV_D
    gencallinterp((uint32_t)MOV_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    mov_reg32_preg32(EBX, EAX);
    mov_reg32_preg32pimm32(ECX, EAX, 4);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    mov_preg32_reg32(EAX, EBX);
    mov_preg32pimm32_reg32(EAX, 4, ECX);
#endif
}

void genneg_d()
{
#ifdef INTERPRET_NEG_D
    gencallinterp((uint32_t)NEG_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fld_preg32_qword(EAX);
    fchs();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    fstp_preg32_qword(EAX);
#endif
}

void genround_l_d()
{
#ifdef INTERPRET_ROUND_L_D
    gencallinterp((uint32_t)ROUND_L_D, 0);
#else
    gencheck_cop1_unusable();
    fldcw_m16((uint16_t*)&round_mode);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    fistp_preg32_qword(EAX);
    fldcw_m16((uint16_t*)&rounding_mode);
    gencheck_float_conversion_valid();
#endif
}

void gentrunc_l_d()
{
#ifdef INTERPRET_TRUNC_L_D
    gencallinterp((uint32_t)TRUNC_L_D, 0);
#else
    gencheck_cop1_unusable();
    fldcw_m16((uint16_t*)&trunc_mode);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    fistp_preg32_qword(EAX);
    fldcw_m16((uint16_t*)&rounding_mode);
    gencheck_float_conversion_valid();
#endif
}

void genceil_l_d()
{
#ifdef INTERPRET_CEIL_L_D
    gencallinterp((uint32_t)CEIL_L_D, 0);
#else
    gencheck_cop1_unusable();
    fldcw_m16((uint16_t*)&ceil_mode);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    fistp_preg32_qword(EAX);
    fldcw_m16((uint16_t*)&rounding_mode);
    gencheck_float_conversion_valid();
#endif
}

void genfloor_l_d()
{
#ifdef INTERPRET_FLOOR_L_D
    gencallinterp((uint32_t)FLOOR_L_D, 0);
#else
    gencheck_cop1_unusable();
    fldcw_m16((uint16_t*)&floor_mode);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    fistp_preg32_qword(EAX);
    fldcw_m16((uint16_t*)&rounding_mode);
    gencheck_float_conversion_valid();
#endif
}

void genround_w_d()
{
#ifdef INTERPRET_ROUND_W_D
    gencallinterp((uint32_t)ROUND_W_D, 0);
#else
    gencheck_cop1_unusable();
    fldcw_m16((uint16_t*)&round_mode);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    fistp_preg32_dword(EAX);
    fldcw_m16((uint16_t*)&rounding_mode);
    gencheck_float_conversion_valid();
#endif
}

void gentrunc_w_d()
{
#ifdef INTERPRET_TRUNC_W_D
    gencallinterp((uint32_t)TRUNC_W_D, 0);
#else
    gencheck_cop1_unusable();
    fldcw_m16((uint16_t*)&trunc_mode);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    fistp_preg32_dword(EAX);
    fldcw_m16((uint16_t*)&rounding_mode);
    gencheck_float_conversion_valid();
#endif
}

void genceil_w_d()
{
#ifdef INTERPRET_CEIL_W_D
    gencallinterp((uint32_t)CEIL_W_D, 0);
#else
    gencheck_cop1_unusable();
    fldcw_m16((uint16_t*)&ceil_mode);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    fistp_preg32_dword(EAX);
    fldcw_m16((uint16_t*)&rounding_mode);
    gencheck_float_conversion_valid();
#endif
}

void genfloor_w_d()
{
#ifdef INTERPRET_FLOOR_W_D
    gencallinterp((uint32_t)FLOOR_W_D, 0);
#else
    gencheck_cop1_unusable();
    fldcw_m16((uint16_t*)&floor_mode);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    fistp_preg32_dword(EAX);
    fldcw_m16((uint16_t*)&rounding_mode);
    gencheck_float_conversion_valid();
#endif
}

void gencvt_s_d()
{
#ifdef INTERPRET_CVT_S_D
    gencallinterp((uint32_t)CVT_S_D, 0);
#else
    gencheck_cop1_unusable();
    if (g_config.wii_vc_emulation)
    {
        fldcw_m16((uint16_t*)&trunc_mode);
    }
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fld_preg32_qword(EAX);
    gencheck_result_valid_s();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    fstp_preg32_dword(EAX);
    if (g_config.wii_vc_emulation)
    {
        fldcw_m16((uint16_t*)&rounding_mode);
    }
#endif
}

void gencvt_w_d()
{
#ifdef INTERPRET_CVT_W_D
    gencallinterp((uint32_t)CVT_W_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_simple[dst->f.cf.fd]));
    fistp_preg32_dword(EAX);
    gencheck_float_conversion_valid();
#endif
}

void gencvt_l_d()
{
#ifdef INTERPRET_CVT_L_D
    gencallinterp((uint32_t)CVT_L_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_eax_valid(0);
    fclex();
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fd]));
    fistp_preg32_qword(EAX);
    gencheck_float_conversion_valid();
#endif
}

void genc_f_d()
{
#ifdef INTERPRET_C_F_D
    gencallinterp((uint32_t)C_F_D, 0);
#else
    gencheck_cop1_unusable();
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000);
#endif
}

void genc_un_d()
{
#ifdef INTERPRET_C_UN_D
    gencallinterp((uint32_t)C_UN_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.ft]));
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    fld_preg32_qword(EAX);
    fucomip_fpreg(1);
    ffree_fpreg(0);
    jp_rj(12);
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
    jmp_imm_short(10); // 2
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
#endif
}

void genc_eq_d()
{
#ifdef INTERPRET_C_EQ_D
    gencallinterp((uint32_t)C_EQ_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.ft]));
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    fld_preg32_qword(EAX);
    fucomip_fpreg(1);
    ffree_fpreg(0);
    jne_rj(12); // 2
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_ueq_d()
{
#ifdef INTERPRET_C_UEQ_D
    gencallinterp((uint32_t)C_UEQ_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.ft]));
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    fld_preg32_qword(EAX);
    fucomip_fpreg(1);
    ffree_fpreg(0);
    jp_rj(14);
    jne_rj(12);
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_olt_d()
{
#ifdef INTERPRET_C_OLT_D
    gencallinterp((uint32_t)C_OLT_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.ft]));
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    fld_preg32_qword(EAX);
    fucomip_fpreg(1);
    ffree_fpreg(0);
    jae_rj(12); // 2
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_ult_d()
{
#ifdef INTERPRET_C_ULT_D
    gencallinterp((uint32_t)C_ULT_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.ft]));
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    fld_preg32_qword(EAX);
    fucomip_fpreg(1);
    ffree_fpreg(0);
    jp_rj(14);
    jae_rj(12); // 2
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_ole_d()
{
#ifdef INTERPRET_C_OLE_D
    gencallinterp((uint32_t)C_OLE_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.ft]));
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    fld_preg32_qword(EAX);
    fucomip_fpreg(1);
    ffree_fpreg(0);
    ja_rj(12); // 2
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_ule_d()
{
#ifdef INTERPRET_C_ULE_D
    gencallinterp((uint32_t)C_ULE_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.ft]));
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    fld_preg32_qword(EAX);
    fucomip_fpreg(1);
    ffree_fpreg(0);
    jp_rj(14);
    ja_rj(12); // 2
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_sf_d()
{
#ifdef INTERPRET_C_SF_D
    gencallinterp((uint32_t)C_SF_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.ft]));
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    fld_preg32_qword(EAX);
    fcomip_fpreg(1);
    ffree_fpreg(0);
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000);
#endif
}

void genc_ngle_d()
{
#ifdef INTERPRET_C_NGLE_D
    gencallinterp((uint32_t)C_NGLE_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.ft]));
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    fld_preg32_qword(EAX);
    fcomip_fpreg(1);
    ffree_fpreg(0);
    jp_rj(12);
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
    jmp_imm_short(10); // 2
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
#endif
}

void genc_seq_d()
{
#ifdef INTERPRET_C_SEQ_D
    gencallinterp((uint32_t)C_SEQ_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.ft]));
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    fld_preg32_qword(EAX);
    fcomip_fpreg(1);
    ffree_fpreg(0);
    jne_rj(12); // 2
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_ngl_d()
{
#ifdef INTERPRET_C_NGL_D
    gencallinterp((uint32_t)C_NGL_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.ft]));
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    fld_preg32_qword(EAX);
    fcomip_fpreg(1);
    ffree_fpreg(0);
    jp_rj(14);
    jne_rj(12);
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_lt_d()
{
#ifdef INTERPRET_C_LT_D
    gencallinterp((uint32_t)C_LT_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.ft]));
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    fld_preg32_qword(EAX);
    fcomip_fpreg(1);
    ffree_fpreg(0);
    jae_rj(12); // 2
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_nge_d()
{
#ifdef INTERPRET_C_NGE_D
    gencallinterp((uint32_t)C_NGE_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.ft]));
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    fld_preg32_qword(EAX);
    fcomip_fpreg(1);
    ffree_fpreg(0);
    jp_rj(14);
    jae_rj(12); // 2
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_le_d()
{
#ifdef INTERPRET_C_LE_D
    gencallinterp((uint32_t)C_LE_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.ft]));
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    fld_preg32_qword(EAX);
    fcomip_fpreg(1);
    ffree_fpreg(0);
    ja_rj(12); // 2
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}

void genc_ngt_d()
{
#ifdef INTERPRET_C_NGT_D
    gencallinterp((uint32_t)C_NGT_D, 0);
#else
    gencheck_cop1_unusable();
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.ft]));
    fld_preg32_qword(EAX);
    mov_eax_memoffs32((uint32_t*)(&reg_cop1_double[dst->f.cf.fs]));
    fld_preg32_qword(EAX);
    fcomip_fpreg(1);
    ffree_fpreg(0);
    jp_rj(14);
    ja_rj(12); // 2
    or_m32_imm32((uint32_t*)&FCR31, 0x800000); // 10
    jmp_imm_short(10); // 2
    and_m32_imm32((uint32_t*)&FCR31, ~0x800000); // 10
#endif
}
