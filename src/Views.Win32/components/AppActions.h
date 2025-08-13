/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

// Throwaway actions which can be spammed get keys as to not clog up the async executor queue
#define ASYNC_KEY_CLOSE_ROM (1)
#define ASYNC_KEY_START_ROM (2)
#define ASYNC_KEY_RESET_ROM (3)
#define ASYNC_KEY_PLAY_MOVIE (4)

/**
 * \brief A module responsible for implementing standard application actions.
 */
namespace AppActions
{
    /**
     * \brief Initializes the module.
     */
    void init();

    /**
     * \brief Adds the standard app actions to the action registry.
     */
    void add();

    void update_core_fast_forward();

    /**
     * \brief Starts loading a ROM from the given path.
     * \param path A path.
     */
    void load_rom_from_path(const std::wstring& path);
} // namespace AppActions
