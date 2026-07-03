/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include <Core.hpp>
#include <Memory/Savestates.hpp>
#include <Memory/StateHash.hpp>

namespace
{
    // FNV-1a 64-bit. Dependency-free and swappable; only used for parity comparison, not cryptography.
    constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    constexpr uint64_t FNV_PRIME = 1099511628211ULL;

    uint64_t fnv1a(const void *data, size_t len, uint64_t hash = FNV_OFFSET_BASIS)
    {
        const auto *bytes = static_cast<const uint8_t *>(data);
        for (size_t i = 0; i < len; ++i)
        {
            hash ^= bytes[i];
            hash *= FNV_PRIME;
        }
        return hash;
    }

    bool g_active{};
    int32_t g_interval{1};
    uint64_t g_running{FNV_OFFSET_BASIS};
    std::vector<std::pair<int32_t, uint64_t>> g_checkpoints{};
}

namespace StateHash
{
    void begin(int32_t interval)
    {
        g_active = true;
        g_interval = interval < 1 ? 1 : interval;
        g_running = FNV_OFFSET_BASIS;
        g_checkpoints.clear();
        g_core->log_info(std::format("[StateHash] Started (interval={} samples)", g_interval));
    }

    void on_sample(int32_t sample)
    {
        if (!g_active || sample % g_interval != 0)
        {
            return;
        }

        const auto st = generate_savestate_for_hash();
        const uint64_t checkpoint = fnv1a(st.data(), st.size());

        // Chain the per-checkpoint hash into the running hash so a single value summarizes the whole run.
        g_running = fnv1a(&checkpoint, sizeof(checkpoint), g_running);
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
            std::format("[StateHash] Final hash: {:016x} ({} checkpoints)", g_running, g_checkpoints.size()));
        for (const auto &[sample, hash] : g_checkpoints)
        {
            g_core->log_info(std::format("[StateHash] sample {} -> {:016x}", sample, hash));
        }
    }

    bool active()
    {
        return g_active;
    }
}
