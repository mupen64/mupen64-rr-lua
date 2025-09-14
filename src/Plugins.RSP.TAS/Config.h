/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

struct t_config {
    int32_t version = 2;
    /**
     * \brief Whether audio lists are processed externally
     */
    int32_t audio_hle = false;
    /**
     * \brief Whether display lists are processed externally
     */
    int32_t graphics_hle = true;
    /**
     * \brief Whether audio lists are processed by the audio plugin specified in audioname
     */
    int32_t audio_external = false;
    /**
     * \brief Verify the cached ucode function on every audio ucode task. Enable this if you are debugging dynamic ucode changes.
     */
    int32_t ucode_cache_verify = false;
    /**
     * \brief Path to the external audio plugin path for alist processing
     */
    wchar_t audio_path[260] = {0};
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
