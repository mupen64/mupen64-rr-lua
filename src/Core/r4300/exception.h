/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

void address_error_exception();
void TLB_invalid_exception();
void TLB_refill_exception(uint32_t addresse, int32_t w);
void TLB_mod_exception();
void integer_overflow_exception();
void coprocessor_unusable_exception();

void exception_general();
