/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

extern bool vcr_reset_requested;

/**
 * \brief Notifies VCR engine about controller being polled
 * \param index The polled controller's index
 * \param input The controller's input data
 */
void vcr_on_controller_poll(int32_t index, core_buttons* input);

/**
 * \brief Notifies VCR engine about a new VI
 */
void vcr_on_vi();

/**
 * HACK: The VCR engine can prevent the core from pausing. Gets whether the core should be allowed to pause.
 */
bool allows_core_pause();

bool is_frame_skipped();

bool vcr_allows_core_pause();
bool vcr_allows_core_unpause();
