/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Core.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Recomph.hpp>
#include <R4300/x86_64/Assemble.hpp>
#include <R4300/x86_64/Gcop1Helpers.hpp>
#include <R4300/Cop1Helpers.hpp>
#include <R4300/Ops.hpp>

// FP JIT port (bit-exact with the interpreter): all COP1 .S ops are SSE-native — ADD/SUB/MUL/DIV,
// ABS/NEG/MOV (integer bit ops), all C.cond.S compares, CVT_D_S, SQRT, and the float->int
// conversions (cvt*2si / cvtsi2* / sqrtsd, MXCSR swap for round/ceil/floor). INTERPRET_* left
// commented out below.
// #define INTERPRET_SQRT_S
// #define INTERPRET_ROUND_L_S
// #define INTERPRET_TRUNC_L_S
// #define INTERPRET_CEIL_L_S
// #define INTERPRET_FLOOR_L_S
// #define INTERPRET_ROUND_W_S
// #define INTERPRET_TRUNC_W_S
// #define INTERPRET_CEIL_W_S
// #define INTERPRET_FLOOR_W_S
// #define INTERPRET_CVT_W_S
// #define INTERPRET_CVT_L_S
// Signaling C.cond.S compares raise fail_float_input on NaN, gated on float_exception_emulation.

// --- SSE single-precision compares (C.cond.S), non-signaling family ---
// ucomiss sets: unordered => PF=1(ZF=ZF=CF=1); ordered less => CF=1; equal => ZF=1.
static void gen_fcr31_set_c()
{
    or_m32_imm32((void *)&FCR31, 0x800000);
}
static void gen_fcr31_clear_c()
{
    and_m32_imm32((void *)&FCR31, ~0x800000);
}
static void gen_cmp_load_s()
{
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    movss_xmm_preg64(0, EAX); // xmm0 = fs
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.ft]));
    movss_xmm_preg64(1, EAX); // xmm1 = ft
    ucomiss_xmm_xmm(0, 1);
}
// C = 1 when `jset` is taken (UN=jp, UEQ=je, ULT=jb, ULE=jbe). Assumes cop1 check already emitted.
static void ccond_setjump_body_s(void (*jset)(unsigned char))
{
    gen_cmp_load_s();
    jset(0);
    int32_t js = code_length - 1;
    gen_fcr31_clear_c();
    jmp_imm_short(0);
    int32_t jd = code_length - 1;
    rj_patch(js);
    gen_fcr31_set_c();
    rj_patch(jd);
}
// C = 0 when jp (unordered) or `jclear2`; else C = 1 (EQ=jne, OLT=jae, OLE=ja). Assumes cop1 check emitted.
static void ccond_clearjump_body_s(void (*jclear2)(unsigned char))
{
    gen_cmp_load_s();
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
// Signaling family: raise fail_float_input on NaN. Only emitted when float_exception_emulation
// is on (matches the interpreter, which now gates the same fail on that toggle).
static void ccond_signaling_nan_s()
{
    gen_cmp_load_s();
    jp_rj(0);
    int32_t jf = code_length - 1;
    jmp_imm_short(0);
    int32_t jsk = code_length - 1;
    rj_patch(jf);
    gencallinterp((uintptr_t)fail_float_input, 0); // returning call
    rj_patch(jsk);
}
static void gen_ccond_setjump_s(void (*jset)(unsigned char))
{
    gencheck_cop1_unusable();
    ccond_setjump_body_s(jset);
}
static void gen_ccond_clearjump_s(void (*jclear2)(unsigned char))
{
    gencheck_cop1_unusable();
    ccond_clearjump_body_s(jclear2);
}
// Signaling ordered compares (LT/NGE=jae, LE/NGT=ja, SEQ/NGL=jne): NaN fail (toggle-gated) + ordered predicate.
static void gen_ccond_sig_clearjump_s(void (*jclear2)(unsigned char))
{
    gencheck_cop1_unusable();
    if (g_core->cfg->float_exception_emulation) ccond_signaling_nan_s();
    ccond_clearjump_body_s(jclear2);
}
// Signaling SF/NGLE: NaN fail (toggle-gated) + always clear.
static void gen_ccond_sig_clear_s()
{
    gencheck_cop1_unusable();
    if (g_core->cfg->float_exception_emulation) ccond_signaling_nan_s();
    gen_fcr31_clear_c();
}

void genadd_s()
{
#ifdef INTERPRET_ADD_S
    gencallinterp((uintptr_t)ADD_S, 0);
#else
    // SSE port: bit-exact with the interpreter (same SSE add + same MXCSR rounding,
    // maintained by the interpreted CTC1 -> fesetround). Denormal/NaN handled by the
    // SSE gencheck_input_s/gencheck_output_s (mirror CHECK_INPUT/CHECK_OUTPUT).
    gencheck_cop1_unusable();
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.ft]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    movss_xmm_preg64(0, EAX); // xmm0 = fs
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.ft]));
    addss_xmm_preg64(0, EAX); // xmm0 += ft
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    movss_preg64_xmm(EAX, 0); // fd = xmm0
    gencheck_output_s((void *)(&reg_cop1_simple[dst->f.cf.fd]));
#endif
}

void gensub_s()
{
#ifdef INTERPRET_SUB_S
    gencallinterp((uintptr_t)SUB_S, 0);
#else
    gencheck_cop1_unusable();
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.ft]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    movss_xmm_preg64(0, EAX);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.ft]));
    subss_xmm_preg64(0, EAX);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    movss_preg64_xmm(EAX, 0);
    gencheck_output_s((void *)(&reg_cop1_simple[dst->f.cf.fd]));
#endif
}

void genmul_s()
{
#ifdef INTERPRET_MUL_S
    gencallinterp((uintptr_t)MUL_S, 0);
#else
    gencheck_cop1_unusable();
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.ft]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    movss_xmm_preg64(0, EAX);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.ft]));
    mulss_xmm_preg64(0, EAX);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    movss_preg64_xmm(EAX, 0);
    gencheck_output_s((void *)(&reg_cop1_simple[dst->f.cf.fd]));
#endif
}

void gendiv_s()
{
#ifdef INTERPRET_DIV_S
    gencallinterp((uintptr_t)DIV_S, 0);
#else
    gencheck_cop1_unusable();
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.ft]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    movss_xmm_preg64(0, EAX); // xmm0 = fs
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.ft]));
    divss_xmm_preg64(0, EAX); // xmm0 /= ft
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    movss_preg64_xmm(EAX, 0); // fd = xmm0
    gencheck_output_s((void *)(&reg_cop1_simple[dst->f.cf.fd]));
#endif
}

void gensqrt_s()
{
#ifdef INTERPRET_SQRT_S
    gencallinterp((uintptr_t)SQRT_S, 0);
#else
    // sqrt in DOUBLE then narrow: interpreter does (float)sqrt((double)fs), not sqrtf.
    gencheck_cop1_unusable();
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    cvtss2sd_xmm_preg64(0, EAX); // xmm0 = (double)fs
    sqrtsd_xmm_xmm(0, 0);        // xmm0 = sqrt(xmm0)
    cvtsd2ss_xmm_xmm(0, 0);      // xmm0 = (float)xmm0
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    movss_preg64_xmm(EAX, 0); // fd = xmm0
    gencheck_output_s((void *)(&reg_cop1_simple[dst->f.cf.fd]));
#endif
}

void genabs_s()
{
#ifdef INTERPRET_ABS_S
    gencallinterp((uintptr_t)ABS_S, 0);
#else
    // fabs = clear the sign bit (bit-exact; ABS cannot fail, no output check).
    gencheck_cop1_unusable();
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    mov_reg32_preg32(EBX, EAX);
    and_reg32_imm32(EBX, 0x7FFFFFFF);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    mov_preg32_reg32(EAX, EBX);
#endif
}

void genmov_s()
{
#ifdef INTERPRET_MOV_S
    gencallinterp((uintptr_t)MOV_S, 0);
#else
    // Raw 32-bit copy (no check). 64-bit pointer loads (the old mov_eax_memoffs32
    // truncated the FPR pointer on x64).
    gencheck_cop1_unusable();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    mov_reg32_preg32(EBX, EAX);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    mov_preg32_reg32(EAX, EBX);
#endif
}

void genneg_s()
{
#ifdef INTERPRET_NEG_S
    gencallinterp((uintptr_t)NEG_S, 0);
#else
    // Negate = flip the sign bit (bit-exact; NEG cannot fail, no output check).
    gencheck_cop1_unusable();
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    mov_reg32_preg32(EBX, EAX);
    xor_reg32_imm32(EBX, 0x80000000);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    mov_preg32_reg32(EAX, EBX);
#endif
}

void genround_l_s()
{
#ifdef INTERPRET_ROUND_L_S
    gencallinterp((uintptr_t)ROUND_L_S, 0);
#else
    GEN_FALLBACK_IF_FLOAT_EXC(ROUND_L_S);
    gencheck_cop1_unusable();
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    gen_mxcsr_set_round(0x0000); // nearest
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    cvtss2si_reg64_preg64(EBX, EAX);
    gen_mxcsr_restore();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    mov_preg64_reg64(EAX, EBX);
#endif
}

void gentrunc_l_s()
{
#ifdef INTERPRET_TRUNC_L_S
    gencallinterp((uintptr_t)TRUNC_L_S, 0);
#else
    GEN_FALLBACK_IF_FLOAT_EXC(TRUNC_L_S);
    gencheck_cop1_unusable();
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    cvttss2si_reg64_preg64(EBX, EAX); // rbx = (int64)trunc(fs)
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    mov_preg64_reg64(EAX, EBX); // fd = rbx
#endif
}

void genceil_l_s()
{
#ifdef INTERPRET_CEIL_L_S
    gencallinterp((uintptr_t)CEIL_L_S, 0);
#else
    GEN_FALLBACK_IF_FLOAT_EXC(CEIL_L_S);
    gencheck_cop1_unusable();
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    gen_mxcsr_set_round(0x4000); // up (+inf)
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    cvtss2si_reg64_preg64(EBX, EAX);
    gen_mxcsr_restore();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    mov_preg64_reg64(EAX, EBX);
#endif
}

void genfloor_l_s()
{
#ifdef INTERPRET_FLOOR_L_S
    gencallinterp((uintptr_t)FLOOR_L_S, 0);
#else
    GEN_FALLBACK_IF_FLOAT_EXC(FLOOR_L_S);
    gencheck_cop1_unusable();
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    gen_mxcsr_set_round(0x2000); // down (-inf)
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    cvtss2si_reg64_preg64(EBX, EAX);
    gen_mxcsr_restore();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    mov_preg64_reg64(EAX, EBX);
#endif
}

void genround_w_s()
{
#ifdef INTERPRET_ROUND_W_S
    gencallinterp((uintptr_t)ROUND_W_S, 0);
#else
    GEN_FALLBACK_IF_FLOAT_EXC(ROUND_W_S);
    gencheck_cop1_unusable();
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    gen_mxcsr_set_round(0x0000); // nearest
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    cvtss2si_reg32_preg64(EBX, EAX);
    gen_mxcsr_restore();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    mov_preg64_reg32(EAX, EBX);
#endif
}

void gentrunc_w_s()
{
#ifdef INTERPRET_TRUNC_W_S
    gencallinterp((uintptr_t)TRUNC_W_S, 0);
#else
    GEN_FALLBACK_IF_FLOAT_EXC(TRUNC_W_S);
    gencheck_cop1_unusable();
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    cvttss2si_reg32_preg64(EBX, EAX); // ebx = (int32)trunc(fs)
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    mov_preg64_reg32(EAX, EBX); // fd = ebx
#endif
}

void genceil_w_s()
{
#ifdef INTERPRET_CEIL_W_S
    gencallinterp((uintptr_t)CEIL_W_S, 0);
#else
    GEN_FALLBACK_IF_FLOAT_EXC(CEIL_W_S);
    gencheck_cop1_unusable();
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    gen_mxcsr_set_round(0x4000); // up (+inf)
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    cvtss2si_reg32_preg64(EBX, EAX);
    gen_mxcsr_restore();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    mov_preg64_reg32(EAX, EBX);
#endif
}

void genfloor_w_s()
{
#ifdef INTERPRET_FLOOR_W_S
    gencallinterp((uintptr_t)FLOOR_W_S, 0);
#else
    GEN_FALLBACK_IF_FLOAT_EXC(FLOOR_W_S);
    gencheck_cop1_unusable();
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    gen_mxcsr_set_round(0x2000); // down (-inf)
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    cvtss2si_reg32_preg64(EBX, EAX);
    gen_mxcsr_restore();
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    mov_preg64_reg32(EAX, EBX);
#endif
}

void gencvt_d_s()
{
#ifdef INTERPRET_CVT_D_S
    gencallinterp((uintptr_t)CVT_D_S, 0);
#else
    // float -> double widening (cvtss2sd). No output check (widening is exact).
    gencheck_cop1_unusable();
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    cvtss2sd_xmm_preg64(0, EAX);
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    movsd_preg64_xmm(EAX, 0);
#endif
}

void gencvt_w_s()
{
#ifdef INTERPRET_CVT_W_S
    gencallinterp((uintptr_t)CVT_W_S, 0);
#else
    GEN_FALLBACK_IF_FLOAT_EXC(CVT_W_S);
    gencheck_cop1_unusable();
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    cvtss2si_reg32_preg64(EBX, EAX); // ebx = (int32)round(fs) per MXCSR
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fd]));
    mov_preg64_reg32(EAX, EBX); // fd = ebx
#endif
}

void gencvt_l_s()
{
#ifdef INTERPRET_CVT_L_S
    gencallinterp((uintptr_t)CVT_L_S, 0);
#else
    GEN_FALLBACK_IF_FLOAT_EXC(CVT_L_S);
    gencheck_cop1_unusable();
    gencheck_input_s((void *)(&reg_cop1_simple[dst->f.cf.fs]));
    mov_reg64_m64(EAX, (void *)(&reg_cop1_simple[dst->f.cf.fs]));
    cvtss2si_reg64_preg64(EBX, EAX); // rbx = (int64)round(fs) per MXCSR
    mov_reg64_m64(EAX, (void *)(&reg_cop1_double[dst->f.cf.fd]));
    mov_preg64_reg64(EAX, EBX); // fd = rbx
#endif
}

void genc_f_s()
{
#ifdef INTERPRET_C_F_S
    gencallinterp((uintptr_t)C_F_S, 0);
#else
    gencheck_cop1_unusable();
    and_m32_imm32((void *)&FCR31, ~0x800000);
#endif
}

void genc_un_s()
{
#ifdef INTERPRET_C_UN_S
    gencallinterp((uintptr_t)C_UN_S, 0);
#else
    gen_ccond_setjump_s(jp_rj); // C = unordered
#endif
}

void genc_eq_s()
{
#ifdef INTERPRET_C_EQ_S
    gencallinterp((uintptr_t)C_EQ_S, 0);
#else
    gen_ccond_clearjump_s(jne_rj); // C = ordered && equal (always NaN-accurate, matches interpreter)
#endif
}

void genc_ueq_s()
{
#ifdef INTERPRET_C_UEQ_S
    gencallinterp((uintptr_t)C_UEQ_S, 0);
#else
    gen_ccond_setjump_s(je_rj); // C = unordered || equal
#endif
}

void genc_olt_s()
{
#ifdef INTERPRET_C_OLT_S
    gencallinterp((uintptr_t)C_OLT_S, 0);
#else
    gen_ccond_clearjump_s(jae_rj); // C = ordered && less
#endif
}

void genc_ult_s()
{
#ifdef INTERPRET_C_ULT_S
    gencallinterp((uintptr_t)C_ULT_S, 0);
#else
    gen_ccond_setjump_s(jb_rj); // C = unordered || less
#endif
}

void genc_ole_s()
{
#ifdef INTERPRET_C_OLE_S
    gencallinterp((uintptr_t)C_OLE_S, 0);
#else
    gen_ccond_clearjump_s(ja_rj); // C = ordered && (less or equal)
#endif
}

void genc_ule_s()
{
#ifdef INTERPRET_C_ULE_S
    gencallinterp((uintptr_t)C_ULE_S, 0);
#else
    gen_ccond_setjump_s(jbe_rj); // C = unordered || (less or equal)
#endif
}

void genc_sf_s()
{
#ifdef INTERPRET_C_SF_S
    gencallinterp((uintptr_t)C_SF_S, 0);
#else
    GEN_FALLBACK_IF_FLOAT_EXC(C_SF_S);
    gen_ccond_sig_clear_s(); // C = 0 (signaling false)
#endif
}

void genc_ngle_s()
{
#ifdef INTERPRET_C_NGLE_S
    gencallinterp((uintptr_t)C_NGLE_S, 0);
#else
    GEN_FALLBACK_IF_FLOAT_EXC(C_NGLE_S);
    gen_ccond_sig_clear_s(); // interpreter clears unconditionally (C = 0)
#endif
}

void genc_seq_s()
{
#ifdef INTERPRET_C_SEQ_S
    gencallinterp((uintptr_t)C_SEQ_S, 0);
#else
    GEN_FALLBACK_IF_FLOAT_EXC(C_SEQ_S);
    gen_ccond_sig_clearjump_s(jne_rj); // ordered equal
#endif
}

void genc_ngl_s()
{
#ifdef INTERPRET_C_NGL_S
    gencallinterp((uintptr_t)C_NGL_S, 0);
#else
    GEN_FALLBACK_IF_FLOAT_EXC(C_NGL_S);
    gen_ccond_sig_clearjump_s(jne_rj); // interpreter: ordered equal (same as SEQ)
#endif
}

void genc_lt_s()
{
#ifdef INTERPRET_C_LT_S
    gencallinterp((uintptr_t)C_LT_S, 0);
#else
    GEN_FALLBACK_IF_FLOAT_EXC(C_LT_S);
    gen_ccond_sig_clearjump_s(jae_rj); // ordered less
#endif
}

void genc_nge_s()
{
#ifdef INTERPRET_C_NGE_S
    gencallinterp((uintptr_t)C_NGE_S, 0);
#else
    GEN_FALLBACK_IF_FLOAT_EXC(C_NGE_S);
    gen_ccond_sig_clearjump_s(jae_rj); // interpreter: ordered less (same as LT)
#endif
}

void genc_le_s()
{
#ifdef INTERPRET_C_LE_S
    gencallinterp((uintptr_t)C_LE_S, 0);
#else
    GEN_FALLBACK_IF_FLOAT_EXC(C_LE_S);
    gen_ccond_sig_clearjump_s(ja_rj); // ordered less-or-equal
#endif
}

void genc_ngt_s()
{
#ifdef INTERPRET_C_NGT_S
    gencallinterp((uintptr_t)C_NGT_S, 0);
#else
    GEN_FALLBACK_IF_FLOAT_EXC(C_NGT_S);
    gen_ccond_sig_clearjump_s(ja_rj); // interpreter: ordered less-or-equal (same as LE)
#endif
}
