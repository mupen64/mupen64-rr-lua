/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include <Core.hpp>
#include <Memory/Savestates.hpp>
#include <Memory/ParityChecker.hpp>
#include <FNV1A.hpp>

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

void begin(int32_t interval)
{
    s_ctx.active = true;
    s_ctx.interval = interval < 1 ? 1 : interval;
    s_ctx.running = FNV1A::FNV_OFFSET_BASIS;
    s_ctx.checkpoints.clear();
    g_core->log_info(std::format("[ParityChecker] Started (interval={} samples)", s_ctx.interval));
}

void on_sample(int32_t sample)
{
    if (!s_ctx.active) [[likely]]
        return;

    const auto result = st_save_pure([=](const core_st_callback_info &info, const std::vector<uint8_t> &buffer) {
        const uint64_t checkpoint = FNV1A::hash(buffer);
        const auto running_span =
            std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(&checkpoint), sizeof(checkpoint));
        s_ctx.running = FNV1A::hash(running_span, s_ctx.running);
        s_ctx.checkpoints.emplace_back(sample, checkpoint);
    });

    if (!result)
    {
        g_core->log_error("[ParityChecker] Failed to save savestate");
        return;
    }
}

void end()
{
    if (!s_ctx.active) [[likely]]
        return;

    s_ctx.active = false;

    g_core->log_info(
        std::format("[ParityChecker] Final hash: {:016x} ({} checkpoints)", s_ctx.running, s_ctx.checkpoints.size()));
    for (const auto &[sample, hash] : s_ctx.checkpoints)
    {
        g_core->log_info(std::format("[ParityChecker] sample {} -> {:016x}", sample, hash));
    }
}

bool active()
{
    return s_ctx.active;
}
} // namespace ParityChecker
