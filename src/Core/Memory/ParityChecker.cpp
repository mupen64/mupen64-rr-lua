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

namespace
{

bool g_active{};
int32_t g_interval{1};
uint64_t g_running{FNV1A::FNV_OFFSET_BASIS};
std::vector<std::pair<int32_t, uint64_t>> g_checkpoints{};
} // namespace

namespace ParityChecker
{
void begin(int32_t interval)
{
    g_active = true;
    g_interval = interval < 1 ? 1 : interval;
    g_running = FNV1A::FNV_OFFSET_BASIS;
    g_checkpoints.clear();
    g_core->log_info(std::format("[ParityChecker] Started (interval={} samples)", g_interval));
}

void on_sample(int32_t sample)
{
    if (!g_active || sample % g_interval != 0)
    {
        return;
    }

    const auto st = generate_savestate_for_hash();
    const uint64_t checkpoint = FNV1A::hash(st.data(), st.size());

    // Chain the per-checkpoint hash into the running hash so a single value summarizes the whole run.
    g_running = FNV1A::hash(&checkpoint, sizeof(checkpoint), g_running);
    g_checkpoints.emplace_back(sample, checkpoint);
}

void end()
{
    if (!g_active)
    {
        return;
    }

    g_active = false;

    g_core->log_info(
        std::format("[ParityChecker] Final hash: {:016x} ({} checkpoints)", g_running, g_checkpoints.size()));
    for (const auto &[sample, hash] : g_checkpoints)
    {
        g_core->log_info(std::format("[ParityChecker] sample {} -> {:016x}", sample, hash));
    }
}

bool active()
{
    return g_active;
}
} // namespace ParityChecker
