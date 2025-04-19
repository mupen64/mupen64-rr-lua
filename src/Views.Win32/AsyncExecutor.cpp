/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <AsyncExecutor.h>
#include <Config.h>
#include <BS_thread_pool.hpp>

static BS::thread_pool pool;

void AsyncExecutor::init()
{
}

void AsyncExecutor::stop()
{
}

void AsyncExecutor::invoke_async(const std::function<void()>& func, size_t key)
{
    if (!g_config.use_async_executor)
    {
        if (g_config.concurrency_fuzzing)
        {
            std::thread([=] {
                Sleep(500);
                func();
            })
            .detach();
        }
        else
        {
            std::thread(func).detach();
        }
        return;
    }

    const auto _ = pool.submit_task(func);
}
