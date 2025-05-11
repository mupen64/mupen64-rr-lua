/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

void* malloc_exec(size_t size);
void* realloc_exec(void* ptr, size_t oldsize, size_t newsize);
void free_exec(void* ptr);
