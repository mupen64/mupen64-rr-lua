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

extern float largest_denormal_float;
extern double largest_denormal_double;

#define GEN_FALLBACK_IF_FLOAT_EXC(op)                                                                                   \
    do                                                                                                                  \
    {                                                                                                                   \
        if (g_core->cfg->float_exception_emulation)                                                                     \
        {                                                                                                               \
            gencallinterp((uintptr_t)(op), 0);                                                                         \
            return;                                                                                                     \
        }                                                                                                              \
    } while (0)
