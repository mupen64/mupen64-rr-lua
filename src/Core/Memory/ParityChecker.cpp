/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Common/CommonPCH.hpp>
#include <Core.hpp>
#include <Memory/Savestates.hpp>
#include <Memory/ParityChecker.hpp>
#include <Common/FNV1A.hpp>

namespace ParityChecker
{

struct Context
{
    bool active;
    int32_t interval;
    uint64_t running;
    std::vector<std::pair<int32_t, uint64_t>> checkpoints;
};

static Context s_ctx;

void start(int32_t interval)
{
    if (s_ctx.active) return;
    s_ctx.active = true;
    s_ctx.interval = std::max(interval, 1);
    s_ctx.running = FNV1A::FNV_OFFSET_BASIS;
    s_ctx.checkpoints.clear();
    g_core->log_info(std::format("[ParityChecker] Started (interval={} samples)", s_ctx.interval));
}

void stop()
{
    if (!s_ctx.active) [[likely]]
        return;

    s_ctx.active = false;

    g_core->log_info(
        std::format("[ParityChecker] Final hash: {:016x} ({} checkpoints)", s_ctx.running, s_ctx.checkpoints.size()));
}

void on_sample(int32_t sample)
{
    if (!s_ctx.active) [[likely]]
        return;

    if (sample % s_ctx.interval != 0) return;

    const auto result = st_sync_hash([=](const uint64_t hash) {
        const auto running_span = std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(&hash), sizeof(hash));
        s_ctx.running = FNV1A::hash(running_span, s_ctx.running);
        s_ctx.checkpoints.emplace_back(sample, hash);
        g_core->log_info(std::format("[ParityChecker] sample {} -> {:016x}", sample, hash));
    });

    if (!result)
    {
        g_core->log_error("[ParityChecker] Failed to save savestate");
        return;
    }
}

bool active()
{
    return s_ctx.active;
}
} // namespace ParityChecker
