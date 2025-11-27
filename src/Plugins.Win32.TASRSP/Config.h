/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

struct t_config
{
    int32_t version = 2;
    /**
     * \brief Verify the cached ucode function on every audio ucode task. Enable this if you are debugging dynamic ucode
     * changes.
     */
    int32_t ucode_cache_verify = false;
};

extern t_config config;

/**
 * \brief Saves the config
 */
void config_save();

/**
 * \brief Loads the config
 */
void config_load();

void config_show_dialog(HWND hwnd);
