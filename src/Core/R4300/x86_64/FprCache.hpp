/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <R4300/Recomp.hpp>

#define FPR_CACHE_PERSIST 1

void fpr_cache_begin(bool dbl);

void fpr_cache_end();

void fpr_cache_flush();

void fpr_cache_reset();

void fpr_cache_gate(uint32_t src);

void fpr_cache_mark_live();

void fpr_cache_reload_live();

int32_t fpr_cache_read(int32_t fgr);

int32_t fpr_cache_write(int32_t fgr);

void fpr_cache_lock(int32_t xmm);
void fpr_cache_unlock_all();

void fpr_cache_spill_live();

bool fpr_cache_empty();
