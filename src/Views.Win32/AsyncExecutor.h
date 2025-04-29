/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

namespace AsyncExecutor
{
    /**
     * \brief Executes a function asynchronously.
     * \param func The function to be executed.
     * \param key The function's key used for deduplication. If not 0, the function will not be queued if another function with the same key is already in the queue.
     */
    void invoke_async(const std::function<void()>& func, size_t key = 0);
} // namespace AsyncExecutor
