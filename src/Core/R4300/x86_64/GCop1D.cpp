/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include <Core.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Recomph.hpp>
#include <R4300/x86_64/Assemble.hpp>
#include <R4300/x86_64/Gcop1Helpers.hpp>
#include <R4300/Cop1Helpers.hpp>
#include <R4300/Ops.hpp>

// FP JIT port: JITed: ADD/SUB/MUL/DIV/SQRT_D, ABS/NEG/MOV_D (integer bit ops), all
// C.cond.D compares, CVT_S_D. The float->int conversions remain interpreted (INTERPRET_* below).
// All COP1 .D conversions now ported to native SSE (cvt*2si, with MXCSR rounding-mode
// swap for round/ceil/floor). Bit-exact with the interpreter.
// #define INTERPRET_ROUND_L_D
// #define INTERPRET_TRUNC_L_D
// #define INTERPRET_CEIL_L_D
// #define INTERPRET_FLOOR_L_D
// #define INTERPRET_ROUND_W_D
// #define INTERPRET_TRUNC_W_D
// #define INTERPRET_CEIL_W_D
// #define INTERPRET_FLOOR_W_D
// #define INTERPRET_CVT_W_D
// #define INTERPRET_CVT_L_D
// All C.cond.D compares (non-signaling + signaling) ported to SSE below.

// (Removed the now-unused x87 conversion-validity helpers gencheck_eax_valid /
//  gencheck_result_valid / gencheck_result_valid_s: every conversion now uses the SSE
//  gencheck_input_d/output_d.)

// --- SSE double-precision compares (C.cond.D), non-signaling family ---
static void gen_fcr31_set_c() { or_m32_imm32((void *)&FCR31, 0x800000); }
static void gen_fcr31_clear_c() { and_m32_imm32((void *)&FCR31, ~0x800000); }
static void gen_cmp_load_d()
{
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    movsd_xmm_preg64(0, EAX); // xmm0 = fs
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.ft]));
    movsd_xmm_preg64(1, EAX); // xmm1 = ft
    ucomisd_xmm_xmm(0, 1);
}
static void ccond_setjump_body_d(void (*jset)(unsigned char))
{
    gen_cmp_load_d();
    jset(0);
    int32_t js = code_length - 1;
    gen_fcr31_clear_c();
    jmp_imm_short(0);
    int32_t jd = code_length - 1;
    rj_patch(js);
    gen_fcr31_set_c();
    rj_patch(jd);
}
static void ccond_clearjump_body_d(void (*jclear2)(unsigned char))
{
    gen_cmp_load_d();
    jp_rj(0);
    int32_t jc1 = code_length - 1;
    jclear2(0);
    int32_t jc2 = code_length - 1;
    gen_fcr31_set_c();
    jmp_imm_short(0);
    int32_t jd = code_length - 1;
    rj_patch(jc1);
    rj_patch(jc2);
    gen_fcr31_clear_c();
    rj_patch(jd);
}
static void ccond_signaling_nan_d()
{
    gen_cmp_load_d();
    jp_rj(0);
    int32_t jf = code_length - 1;
    jmp_imm_short(0);
    int32_t jsk = code_length - 1;
    rj_patch(jf);
    gencallinterp((uintptr_t)fail_float_input, 0);
    rj_patch(jsk);
}
static void gen_ccond_setjump_d(void (*jset)(unsigned char)) { gencheck_cop1_unusable(); ccond_setjump_body_d(jset); }
static void gen_ccond_clearjump_d(void (*jclear2)(unsigned char)) { gencheck_cop1_unusable(); ccond_clearjump_body_d(jclear2); }
static void gen_ccond_sig_clearjump_d(void (*jclear2)(unsigned char))
{
    gencheck_cop1_unusable();
    if (g_core->cfg->float_exception_emulation) ccond_signaling_nan_d();
    ccond_clearjump_body_d(jclear2);
}
static void gen_ccond_sig_clear_d()
{
    gencheck_cop1_unusable();
    if (g_core->cfg->float_exception_emulation) ccond_signaling_nan_d();
    gen_fcr31_clear_c();
}

void genadd_d()
{
#ifdef INTERPRET_ADD_D
    gencallinterp((uintptr_t)ADD_D, 0);
#else
    gencheck_cop1_unusable();
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.ft]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    movsd_xmm_preg64(0, EAX);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.ft]));
    addsd_xmm_preg64(0, EAX);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    movsd_preg64_xmm(EAX, 0);
    gencheck_output_d((void *)(&reg_cop1_double[dst->f.cf.fd]));
#endif
}

void gensub_d()
{
#ifdef INTERPRET_SUB_D
    gencallinterp((uintptr_t)SUB_D, 0);
#else
    gencheck_cop1_unusable();
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.ft]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    movsd_xmm_preg64(0, EAX);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.ft]));
    subsd_xmm_preg64(0, EAX);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    movsd_preg64_xmm(EAX, 0);
    gencheck_output_d((void *)(&reg_cop1_double[dst->f.cf.fd]));
#endif
}

void genmul_d()
{
#ifdef INTERPRET_MUL_D
    gencallinterp((uintptr_t)MUL_D, 0);
#else
    gencheck_cop1_unusable();
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.ft]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    movsd_xmm_preg64(0, EAX);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.ft]));
    mulsd_xmm_preg64(0, EAX);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    movsd_preg64_xmm(EAX, 0);
    gencheck_output_d((void *)(&reg_cop1_double[dst->f.cf.fd]));
#endif
}

void gendiv_d()
{
#ifdef INTERPRET_DIV_D
    gencallinterp((uintptr_t)DIV_D, 0);
#else
    gencheck_cop1_unusable();
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.fs]));
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.ft]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    movsd_xmm_preg64(0, EAX);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.ft]));
    divsd_xmm_preg64(0, EAX);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    movsd_preg64_xmm(EAX, 0);
    gencheck_output_d((void *)(&reg_cop1_double[dst->f.cf.fd]));
#endif
}

void gensqrt_d()
{
#ifdef INTERPRET_SQRT_D
    gencallinterp((uintptr_t)SQRT_D, 0);
#else
    // sqrt(double) is unambiguously double precision -> sqrtsd (bit-exact).
    gencheck_cop1_unusable();
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.fs]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    sqrtsd_xmm_preg64(0, EAX);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    movsd_preg64_xmm(EAX, 0);
    gencheck_output_d((void *)(&reg_cop1_double[dst->f.cf.fd]));
#endif
}

void genabs_d()
{
#ifdef INTERPRET_ABS_D
    gencallinterp((uintptr_t)ABS_D, 0);
#else
    // fabs = clear the sign bit (bit 63 = bit 31 of the high dword). ABS cannot fail.
    gencheck_cop1_unusable();
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.fs]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    mov_reg32_preg32(EBX, EAX);
    mov_reg32_preg32pimm32(ECX, EAX, 4);
    and_reg32_imm32(ECX, 0x7FFFFFFF);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    mov_preg32_reg32(EAX, EBX);
    mov_preg32pimm32_reg32(EAX, 4, ECX);
#endif
}

void genmov_d()
{
#ifdef INTERPRET_MOV_D
    gencallinterp((uintptr_t)MOV_D, 0);
#else
    // Raw 64-bit copy (two dwords), no check. 64-bit pointer loads.
    gencheck_cop1_unusable();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    mov_reg32_preg32(EBX, EAX);
    mov_reg32_preg32pimm32(ECX, EAX, 4);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    mov_preg32_reg32(EAX, EBX);
    mov_preg32pimm32_reg32(EAX, 4, ECX);
#endif
}

void genneg_d()
{
#ifdef INTERPRET_NEG_D
    gencallinterp((uintptr_t)NEG_D, 0);
#else
    // Negate = flip the sign bit (bit 63 = bit 31 of the high dword). NEG cannot fail.
    gencheck_cop1_unusable();
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.fs]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    mov_reg32_preg32(EBX, EAX);
    mov_reg32_preg32pimm32(ECX, EAX, 4);
    xor_reg32_imm32(ECX, 0x80000000);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    mov_preg32_reg32(EAX, EBX);
    mov_preg32pimm32_reg32(EAX, 4, ECX);
#endif
}

void genround_l_d()
{
#ifdef INTERPRET_ROUND_L_D
    gencallinterp((uintptr_t)ROUND_L_D, 0);
#else
    gencheck_cop1_unusable();
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.fs]));
    gen_mxcsr_set_round(0x0000); // nearest
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    cvtsd2si_reg64_preg64(EBX, EAX);
    gen_mxcsr_restore();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    mov_preg64_reg64(EAX, EBX);
#endif
}

void gentrunc_l_d()
{
#ifdef INTERPRET_TRUNC_L_D
    gencallinterp((uintptr_t)TRUNC_L_D, 0);
#else
    gencheck_cop1_unusable();
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.fs]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    cvttsd2si_reg64_preg64(EBX, EAX); // rbx = (int64)trunc(fs)
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    mov_preg64_reg64(EAX, EBX);       // fd = rbx
#endif
}

void genceil_l_d()
{
#ifdef INTERPRET_CEIL_L_D
    gencallinterp((uintptr_t)CEIL_L_D, 0);
#else
    gencheck_cop1_unusable();
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.fs]));
    gen_mxcsr_set_round(0x4000); // up (+inf)
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    cvtsd2si_reg64_preg64(EBX, EAX);
    gen_mxcsr_restore();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    mov_preg64_reg64(EAX, EBX);
#endif
}

void genfloor_l_d()
{
#ifdef INTERPRET_FLOOR_L_D
    gencallinterp((uintptr_t)FLOOR_L_D, 0);
#else
    gencheck_cop1_unusable();
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.fs]));
    gen_mxcsr_set_round(0x2000); // down (-inf)
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    cvtsd2si_reg64_preg64(EBX, EAX);
    gen_mxcsr_restore();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    mov_preg64_reg64(EAX, EBX);
#endif
}

void genround_w_d()
{
#ifdef INTERPRET_ROUND_W_D
    gencallinterp((uintptr_t)ROUND_W_D, 0);
#else
    gencheck_cop1_unusable();
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.fs]));
    gen_mxcsr_set_round(0x0000); // nearest
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    cvtsd2si_reg32_preg64(EBX, EAX);
    gen_mxcsr_restore();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    mov_preg64_reg32(EAX, EBX);
#endif
}

void gentrunc_w_d()
{
#ifdef INTERPRET_TRUNC_W_D
    gencallinterp((uintptr_t)TRUNC_W_D, 0);
#else
    gencheck_cop1_unusable();
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.fs]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    cvttsd2si_reg32_preg64(EBX, EAX); // ebx = (int32)trunc(fs)
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    mov_preg64_reg32(EAX, EBX);       // fd = ebx
#endif
}

void genceil_w_d()
{
#ifdef INTERPRET_CEIL_W_D
    gencallinterp((uintptr_t)CEIL_W_D, 0);
#else
    gencheck_cop1_unusable();
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.fs]));
    gen_mxcsr_set_round(0x4000); // up (+inf)
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    cvtsd2si_reg32_preg64(EBX, EAX);
    gen_mxcsr_restore();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    mov_preg64_reg32(EAX, EBX);
#endif
}

void genfloor_w_d()
{
#ifdef INTERPRET_FLOOR_W_D
    gencallinterp((uintptr_t)FLOOR_W_D, 0);
#else
    gencheck_cop1_unusable();
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.fs]));
    gen_mxcsr_set_round(0x2000); // down (-inf)
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    cvtsd2si_reg32_preg64(EBX, EAX);
    gen_mxcsr_restore();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    mov_preg64_reg32(EAX, EBX);
#endif
}

void gencvt_s_d()
{
#ifdef INTERPRET_CVT_S_D
    gencallinterp((uintptr_t)CVT_S_D, 0);
#else
    // WiiVC emulation forces truncation for this narrowing; keep that case interpreted.
    if (g_core->cfg->wii_vc_emulation)
    {
        gencallinterp((uintptr_t)CVT_S_D, 0);
        return;
    }
    // double -> float narrowing (cvtsd2ss, rounds per MXCSR = the game's FCR31 mode).
    gencheck_cop1_unusable();
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.fs]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    cvtsd2ss_xmm_preg64(0, EAX);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    movss_preg64_xmm(EAX, 0);
    gencheck_output_s((void *)(&reg_cop1_simple[dst->f.cf.fd]));
#endif
}

void gencvt_w_d()
{
#ifdef INTERPRET_CVT_W_D
    gencallinterp((uintptr_t)CVT_W_D, 0);
#else
    gencheck_cop1_unusable();
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.fs]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    cvtsd2si_reg32_preg64(EBX, EAX); // ebx = (int32)round(fs)
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    mov_preg64_reg32(EAX, EBX);      // fd = ebx
#endif
}

void gencvt_l_d()
{
#ifdef INTERPRET_CVT_L_D
    gencallinterp((uintptr_t)CVT_L_D, 0);
#else
    gencheck_cop1_unusable();
    gencheck_input_d((void *)(&reg_cop1_double[dst->f.cf.fs]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fs]));
    cvtsd2si_reg64_preg64(EBX, EAX); // rbx = (int64)round(fs)
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    mov_preg64_reg64(EAX, EBX);      // fd = rbx
#endif
}

void genc_f_d()
{
#ifdef INTERPRET_C_F_D
    gencallinterp((uintptr_t)C_F_D, 0);
#else
    gencheck_cop1_unusable();
    and_m32_imm32((void *)&FCR31, ~0x800000);
#endif
}

void genc_un_d()
{
#ifdef INTERPRET_C_UN_D
    gencallinterp((uintptr_t)C_UN_D, 0);
#else
    gen_ccond_setjump_d(jp_rj); // C = unordered
#endif
}

void genc_eq_d()
{
#ifdef INTERPRET_C_EQ_D
    gencallinterp((uintptr_t)C_EQ_D, 0);
#else
    gen_ccond_clearjump_d(jne_rj); // C = ordered && equal
#endif
}

void genc_ueq_d()
{
#ifdef INTERPRET_C_UEQ_D
    gencallinterp((uintptr_t)C_UEQ_D, 0);
#else
    gen_ccond_setjump_d(je_rj); // C = unordered || equal
#endif
}

void genc_olt_d()
{
#ifdef INTERPRET_C_OLT_D
    gencallinterp((uintptr_t)C_OLT_D, 0);
#else
    gen_ccond_clearjump_d(jae_rj); // C = ordered && less
#endif
}

void genc_ult_d()
{
#ifdef INTERPRET_C_ULT_D
    gencallinterp((uintptr_t)C_ULT_D, 0);
#else
    gen_ccond_setjump_d(jb_rj); // C = unordered || less
#endif
}

void genc_ole_d()
{
#ifdef INTERPRET_C_OLE_D
    gencallinterp((uintptr_t)C_OLE_D, 0);
#else
    gen_ccond_clearjump_d(ja_rj); // C = ordered && (less or equal)
#endif
}

void genc_ule_d()
{
#ifdef INTERPRET_C_ULE_D
    gencallinterp((uintptr_t)C_ULE_D, 0);
#else
    gen_ccond_setjump_d(jbe_rj); // C = unordered || (less or equal)
#endif
}

void genc_sf_d()
{
#ifdef INTERPRET_C_SF_D
    gencallinterp((uintptr_t)C_SF_D, 0);
#else
    gen_ccond_sig_clear_d(); // C = 0 (signaling false)
#endif
}

void genc_ngle_d()
{
#ifdef INTERPRET_C_NGLE_D
    gencallinterp((uintptr_t)C_NGLE_D, 0);
#else
    gen_ccond_sig_clear_d(); // interpreter clears unconditionally (C = 0)
#endif
}

void genc_seq_d()
{
#ifdef INTERPRET_C_SEQ_D
    gencallinterp((uintptr_t)C_SEQ_D, 0);
#else
    gen_ccond_sig_clearjump_d(jne_rj); // ordered equal
#endif
}

void genc_ngl_d()
{
#ifdef INTERPRET_C_NGL_D
    gencallinterp((uintptr_t)C_NGL_D, 0);
#else
    gen_ccond_sig_clearjump_d(jne_rj); // interpreter: ordered equal (same as SEQ)
#endif
}

void genc_lt_d()
{
#ifdef INTERPRET_C_LT_D
    gencallinterp((uintptr_t)C_LT_D, 0);
#else
    gen_ccond_sig_clearjump_d(jae_rj); // ordered less
#endif
}

void genc_nge_d()
{
#ifdef INTERPRET_C_NGE_D
    gencallinterp((uintptr_t)C_NGE_D, 0);
#else
    gen_ccond_sig_clearjump_d(jae_rj); // interpreter: ordered less (same as LT)
#endif
}

void genc_le_d()
{
#ifdef INTERPRET_C_LE_D
    gencallinterp((uintptr_t)C_LE_D, 0);
#else
    gen_ccond_sig_clearjump_d(ja_rj); // ordered less-or-equal
#endif
}

void genc_ngt_d()
{
#ifdef INTERPRET_C_NGT_D
    gencallinterp((uintptr_t)C_NGT_D, 0);
#else
    gen_ccond_sig_clearjump_d(ja_rj); // interpreter: ordered less-or-equal (same as LE)
#endif
}
