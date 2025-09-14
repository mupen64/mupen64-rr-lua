/*
 * Copyright (c) 2025, hacktarux-azimer-rsp-hle maintainers, contributors, and original authors (Hacktarux, Azimer).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#define PLUGIN_VERSION "1.0.0"

#ifdef _M_X64
#define PLUGIN_ARCH " x64"
#else
#define PLUGIN_ARCH " x86"
#endif

#ifdef _DEBUG
#define PLUGIN_TARGET " Debug"
#else
#define PLUGIN_TARGET " Release"
#endif

#define PLUGIN_ISAEXT " SSE2"

#define PLUGIN_NAME "TAS RSP " PLUGIN_VERSION PLUGIN_ARCH PLUGIN_ISAEXT PLUGIN_TARGET

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
extern int32_t g_config_readonly;

/**
 * \brief Saves the config
 */
void config_save();

/**
 * \brief Loads the config
 */
void config_load();

void config_show_dialog(HWND hwnd);
