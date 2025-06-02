/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <r4300/rom.h>

typedef std::chrono::high_resolution_clock::time_point time_point;

/**
 * \brief To be called when a new frame is generated
 */
void timer_new_frame();

/**
 * \brief To be called when a VI is generated
 */
void timer_new_vi();

void vr_on_speed_modifier_changed();
