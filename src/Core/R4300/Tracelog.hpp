/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
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

bool tl_active();

void tl_start(std::filesystem::path path, bool binary, bool append);
void tl_stop();
