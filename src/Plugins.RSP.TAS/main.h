/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "win.h"

bool rsp_alive();
void on_rom_closed();
uint32_t do_rsp_cycles(uint32_t Cycles);