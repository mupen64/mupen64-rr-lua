/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief Core-side parity/desync tester (issue #407).
 *
 * Every `interval` movie samples during playback, a clean, sync-only savestate is generated (see
 * generate_savestate_for_hash) and hashed with a chained FNV-1a 64-bit hash. Both a running hash over the whole
 * run and the per-checkpoint hashes are kept, so that comparing two builds (e.g. the x86 and x64 recompilers)
 * yields both a single final-hash mismatch signal and the first diverging sample.
 *
 * The comparison itself is external: run the same movie on two builds with hashing enabled and diff the logged
 * hashes. Hooked from the VCR playback tick and finalized at movie end; driven by the `--state-hash` CLI flag.
 */
namespace StateHash
{
    /**
     * \brief Begins a hashing run, resetting the running hash and checkpoints.
     * \param interval Sample interval between checkpoints. Values below 1 are clamped to 1.
     */
    void begin(int32_t interval);

    /**
     * \brief Records a checkpoint if the current sample lands on the interval. No-op when inactive.
     * \param sample The current movie sample.
     */
    void on_sample(int32_t sample);

    /**
     * \brief Finalizes the run and logs the final hash and per-checkpoint hashes. No-op when inactive.
     */
    void end();

    /**
     * \brief Gets whether a hashing run is currently active.
     */
    bool active();
}
