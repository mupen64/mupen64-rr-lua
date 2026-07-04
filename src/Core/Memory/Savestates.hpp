/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <include/core_api.h>

extern bool g_st_skip_dma;
extern bool g_st_old;

/**
 * \brief Does the pending savestate work.
 * \warning This function must only be called from the emulation thread. Other callers must use the
 * <c>savestates_do_x</c> family.
 */
void st_do_work();

/**
 * Clears the work queue and the undo savestate.
 */
void st_on_core_stop();

/**
 * \brief Generates a savestate buffer suitable for parity/desync hashing.
 *
 * The buffer contains only sync-relevant state: the movie freeze buffer and the screenshot are omitted, and no
 * emulation-mutating side effects are performed, so calling this repeatedly during playback does not perturb
 * emulation. See ParityChecker.
 */
std::vector<uint8_t> generate_savestate_for_hash();

bool st_do_file(const std::filesystem::path &path, core_st_job job, const core_st_callback &callback,
                bool ignore_warnings);
bool st_do_memory(const std::vector<uint8_t> &buffer, core_st_job job, const core_st_callback &callback,
                  bool ignore_warnings);
void st_get_undo_savestate(std::vector<uint8_t> &buffer);
