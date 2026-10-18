/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <cstddef>

void *malloc_exec(size_t size);
void *realloc_exec(void *ptr, size_t oldsize, size_t newsize);
void free_exec(void *ptr);

void exec_write_begin();
void exec_write_end();

class ExecWriteScope
{
  public:
    ExecWriteScope();
    ~ExecWriteScope();

    ExecWriteScope(const ExecWriteScope &) = delete;
    ExecWriteScope &operator=(const ExecWriteScope &) = delete;
    ExecWriteScope(ExecWriteScope &&) = delete;
    ExecWriteScope &operator=(ExecWriteScope &&) = delete;
};

// Frees all buffers that were superseded by realloc_exec but could not be
// freed immediately because emitted code may still reference them. Must be
// called when the emulator core shuts down and no dynarec code is executing.
void free_all_deferred_exec_buffers();