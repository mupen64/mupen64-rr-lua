/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <ThreadPool.h>
#include <BS_thread_pool.hpp>

static BS::thread_pool pool(4);
static std::unordered_set<size_t> pending_keys{};
static std::mutex mtx{};

void ThreadPool::submit_task(const std::function<void()> &func, const size_t key)
{
    const auto thunk = [=] {
        try
        {
            func();
        }
        catch (const std::exception &e)
        {
            g_view_logger->critical("[ThreadPool] Unknown exception in task: {}", e.what());
            RaiseException(0xDEADBEEF, EXCEPTION_NONCONTINUABLE, 0, nullptr);
        }
        catch (...)
        {
            g_view_logger->critical("[ThreadPool] Unknown exception in task: unknown exception");
            RaiseException(0xDEADBEEF, EXCEPTION_NONCONTINUABLE, 0, nullptr);
        }
    };

    if (!key)
    {
        (void)pool.submit_task(thunk);
        return;
    }

    {
        std::lock_guard lock(mtx);
        if (!pending_keys.insert(key).second)
        {
            g_view_logger->trace("[AsyncExecutor] Function with key {} already queued, skipping.", key);
            return;
        }
    }

    (void)pool.submit_task([=] {
        thunk();
        {
            std::lock_guard lock(mtx);
            pending_keys.erase(key);
        }
    });
}
