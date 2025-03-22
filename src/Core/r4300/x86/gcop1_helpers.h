/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

void gencheck_float_input_valid(int32_t stackBase);
void gencheck_float_output_valid();
void gencheck_float_conversion_valid();

extern float largest_denormal_float;
extern double largest_denormal_double;
