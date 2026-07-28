/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

// SSE denormal/NaN checks (FP JIT port). fpr_slot == &reg_cop1_simple[X] / &reg_cop1_double[X].
void gencheck_input_s(void *fpr_slot);
void gencheck_output_s(void *fpr_slot);
void gencheck_input_d(void *fpr_slot);
void gencheck_output_d(void *fpr_slot);

// ROUND/CEIL/FLOOR SSE rounding-mode swap (MXCSR). rc_bits: nearest=0, floor=0x2000, ceil=0x4000.
void gen_mxcsr_set_round(uint32_t rc_bits);
void gen_mxcsr_restore();

// Scratch words the generated code stmxcsr/ldmxcsr through when it swaps the SSE rounding
// mode, mirroring the interpreter's fesetround(mode)/set_rounding() around a convert.
// They are written by emitted code only, hence the fixed addresses baked into it.
extern uint32_t g_saved_mxcsr, g_scratch_mxcsr;

// Rounding modes indexed by the N64 mode in FCR31[1:0]; used by genctc1 to keep the
// interpreter's `rounding_mode` global in sync with what CTC1 just wrote to FCR31.
extern const int32_t g_n64_rounding_modes[4];

extern float largest_denormal_float;
extern double largest_denormal_double;

#define GEN_FALLBACK_IF_FLOAT_EXC(op)                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        if (g_core->cfg->float_exception_emulation)                                                                    \
        {                                                                                                              \
            gencallinterp((uintptr_t)(op), 0);                                                                         \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)
