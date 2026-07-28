/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <cstddef>

void *malloc_exec(size_t size);
void *realloc_exec(void *ptr, size_t oldsize, size_t newsize);
void free_exec(void *ptr);

// Frees all buffers that were superseded by realloc_exec but could not be
// freed immediately because emitted code may still reference them. Must be
// called when the emulator core shuts down and no dynarec code is executing.
void free_all_deferred_exec_buffers();