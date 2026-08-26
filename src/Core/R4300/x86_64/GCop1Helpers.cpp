/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Common/CommonPCH.hpp>
#include <Core.hpp>
#include <R4300/Cop1Helpers.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Recomph.hpp>
#include <R4300/x86_64/Assemble.hpp>
#include <R4300/x86_64/Gcop1Helpers.hpp>

uint32_t g_saved_mxcsr = 0, g_scratch_mxcsr = 0;

const int32_t g_n64_rounding_modes[4] = {MUP_ROUND_NEAREST, MUP_ROUND_TRUNC, MUP_ROUND_CEIL, MUP_ROUND_FLOOR};

static void patch_jump(uint32_t addr, uint32_t target)
{
    int32_t diff = target - addr;
    if (diff < -128 || diff > 127)
    {
        g_core->log_error(std::format("[Dynarec] FATAL: patch_jump rel8 out of range ({}) at addr={}", diff, addr));
        abort();
    }
    (*inst_pointer)[addr - 1] = (unsigned char)(diff & 0xFF);
}

static void gencall_noret(void (*fn)())
{
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst));
    mov_reg64_imm64(EAX, (uintptr_t)fn);
    call_reg64(EAX);
    ud2();
}

// --- SSE single-precision denormal/NaN checks (FP JIT port) ---
// Bit masks to compute abs()/copysign() on a 32-bit float held in an xmm register.
static uint32_t sse_abs_mask_s = 0x7FFFFFFF;
static uint32_t sse_sign_mask_s = 0x80000000;

// Mirrors CHECK_INPUT for the single-precision input at *(&reg_cop1_simple[X]) == fpr_slot.
// Fails (fail_float_input) on denormal-nonzero or NaN; passes normal/inf/zero. Gated on
// float_exception_emulation at compile time, like the x87 helpers. Clobbers EAX/EBX, xmm1-3.
void gencheck_input_s(void *fpr_slot)
{
    if (!g_core->cfg->float_exception_emulation) return;

    mov_reg64_m64(EAX, fpr_slot); // EAX = float* (the FPR pointer)
    movss_xmm_preg64(1, EAX);     // xmm1 = x
    mov_reg64_imm64(EBX, (uintptr_t)&sse_abs_mask_s);
    movss_xmm_preg64(2, EBX);
    andps_xmm_xmm(1, 2); // xmm1 = |x|
    mov_reg64_imm64(EBX, (uintptr_t)&largest_denormal_float);
    movss_xmm_preg64(3, EBX);
    ucomiss_xmm_xmm(1, 3);
    ja_rj(0); // |x| > largest -> pass (normal/inf)
    uint32_t jpass1 = code_length;
    xorps_xmm_xmm(2, 2); // xmm2 = 0
    ucomiss_xmm_xmm(1, 2);
    jp_rj(0); // NaN -> fail
    uint32_t jfail = code_length;
    je_rj(0); // |x| == 0 -> pass (zero)
    uint32_t jpass2 = code_length;
    // denormal-nonzero falls through here -> fail (NaN jumps here too)
    patch_jump(jfail, code_length);
    gencall_noret(fail_float_input);
    patch_jump(jpass1, code_length);
    patch_jump(jpass2, code_length);
}

// Mirrors CHECK_OUTPUT for the single-precision result at fpr_slot. When |x| <= largest:
// NaN -> fail_float_output; otherwise flush x to copysign(0, x) in place. Normal/inf unchanged.
void gencheck_output_s(void *fpr_slot)
{
    if (!g_core->cfg->float_exception_emulation) return;

    mov_reg64_m64(EAX, fpr_slot); // EAX = float* (fd pointer)
    movss_xmm_preg64(1, EAX);     // xmm1 = x
    mov_reg64_imm64(EBX, (uintptr_t)&sse_abs_mask_s);
    movss_xmm_preg64(2, EBX);
    andps_xmm_xmm(1, 2); // xmm1 = |x|
    mov_reg64_imm64(EBX, (uintptr_t)&largest_denormal_float);
    movss_xmm_preg64(3, EBX);
    ucomiss_xmm_xmm(1, 3);
    ja_rj(0); // |x| > largest -> done (no change)
    uint32_t jdone1 = code_length;
    jp_rj(0); // NaN -> fail
    uint32_t jfail = code_length;
    // denormal/zero -> flush to copysign(0, x):
    movss_xmm_preg64(2, EAX); // xmm2 = original x (keeps sign)
    mov_reg64_imm64(EBX, (uintptr_t)&sse_sign_mask_s);
    movss_xmm_preg64(3, EBX);
    andps_xmm_xmm(2, 3);      // xmm2 = sign(x) = +-0.0
    movss_preg64_xmm(EAX, 2); // store +-0.0 -> *fd
    jmp_imm_short(0);         // -> done
    uint32_t jdone2 = code_length;
    // fail:
    patch_jump(jfail, code_length);
    gencall_noret(fail_float_output);
    // done:
    patch_jump(jdone1, code_length);
    patch_jump(jdone2, code_length);
}

// --- SSE double-precision denormal/NaN checks (FP JIT port) ---
static uint64_t sse_abs_mask_d = 0x7FFFFFFFFFFFFFFFULL;
static uint64_t sse_sign_mask_d = 0x8000000000000000ULL;

void gencheck_input_d(void *fpr_slot)
{
    if (!g_core->cfg->float_exception_emulation) return;

    mov_reg64_m64(EAX, fpr_slot); // EAX = double* (the FPR pointer)
    movsd_xmm_preg64(1, EAX);     // xmm1 = x
    mov_reg64_imm64(EBX, (uintptr_t)&sse_abs_mask_d);
    movsd_xmm_preg64(2, EBX);
    andpd_xmm_xmm(1, 2); // xmm1 = |x|
    mov_reg64_imm64(EBX, (uintptr_t)&largest_denormal_double);
    movsd_xmm_preg64(3, EBX);
    ucomisd_xmm_xmm(1, 3);
    ja_rj(0); // |x| > largest -> pass
    uint32_t jpass1 = code_length;
    xorps_xmm_xmm(2, 2); // xmm2 = 0
    ucomisd_xmm_xmm(1, 2);
    jp_rj(0); // NaN -> fail
    uint32_t jfail = code_length;
    je_rj(0); // |x| == 0 -> pass
    uint32_t jpass2 = code_length;
    patch_jump(jfail, code_length);
    gencall_noret(fail_float_input);
    patch_jump(jpass1, code_length);
    patch_jump(jpass2, code_length);
}

void gencheck_output_d(void *fpr_slot)
{
    if (!g_core->cfg->float_exception_emulation) return;

    mov_reg64_m64(EAX, fpr_slot); // EAX = double* (fd pointer)
    movsd_xmm_preg64(1, EAX);     // xmm1 = x
    mov_reg64_imm64(EBX, (uintptr_t)&sse_abs_mask_d);
    movsd_xmm_preg64(2, EBX);
    andpd_xmm_xmm(1, 2); // xmm1 = |x|
    mov_reg64_imm64(EBX, (uintptr_t)&largest_denormal_double);
    movsd_xmm_preg64(3, EBX);
    ucomisd_xmm_xmm(1, 3);
    ja_rj(0); // |x| > largest -> done
    uint32_t jdone1 = code_length;
    jp_rj(0); // NaN -> fail
    uint32_t jfail = code_length;
    // denormal/zero -> flush to copysign(0, x):
    movsd_xmm_preg64(2, EAX); // xmm2 = original x (keeps sign)
    mov_reg64_imm64(EBX, (uintptr_t)&sse_sign_mask_d);
    movsd_xmm_preg64(3, EBX);
    andpd_xmm_xmm(2, 3);      // xmm2 = sign(x) = +-0.0
    movsd_preg64_xmm(EAX, 2); // store +-0.0 -> *fd
    jmp_imm_short(0);         // -> done
    uint32_t jdone2 = code_length;
    // fail:
    patch_jump(jfail, code_length);
    gencall_noret(fail_float_output);
    // done:
    patch_jump(jdone1, code_length);
    patch_jump(jdone2, code_length);
}

// ROUND/CEIL/FLOOR: temporarily force the SSE rounding mode (MXCSR RC bits 13-14), then a
// cvt*2si converts with it -- the x64 mirror of the x86 fldcw+fistp path and of the
// interpreter's fesetround(mode)+_mm_cvt. rc_bits: nearest=0, floor(down)=0x2000, ceil(up)=0x4000.
// stmxcsr/ldmxcsr/and/or all clobber only r11 + memory, so EAX/EBX survive across a convert.
void gen_mxcsr_set_round(uint32_t rc_bits)
{
    stmxcsr_m32(&g_saved_mxcsr);               // save the current (FCR31) mode
    stmxcsr_m32(&g_scratch_mxcsr);             // working copy
    and_m32_imm32(&g_scratch_mxcsr, ~0x6000u); // clear the RC field
    if (rc_bits) or_m32_imm32(&g_scratch_mxcsr, rc_bits);
    ldmxcsr_m32(&g_scratch_mxcsr); // apply the fixed rounding mode
}
void gen_mxcsr_restore()
{
    ldmxcsr_m32(&g_saved_mxcsr); // restore the FCR31 mode
}
