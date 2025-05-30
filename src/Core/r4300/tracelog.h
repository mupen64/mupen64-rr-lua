/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief Logs a dynarec-generated instruction
 */
void tracelog_log_interp_ops();

/**
 * \brief Logs a pure interp instruction
 */
void tracelog_log_pure();

bool vr_is_tracelog_active();

void tl_start(std::filesystem::path path, bool binary, bool append);
void tl_stop();
