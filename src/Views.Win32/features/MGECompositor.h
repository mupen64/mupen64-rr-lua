/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once


namespace MGECompositor
{
    /**
     * \brief Initializes the MGE compositor
     */
    void init();

    /**
     * \brief Creates the MGE control
     * \param hwnd The control's parent
     */
    void create(HWND hwnd);

    /**
     * \brief Updates the game screen
     */
    void update_screen();

    /**
     * Gets the current video size from the MGE compositor
     * \param width The video width. If null, the width is not written.
     * \param height The video height. If null, the height is not written.
     */
    void get_video_size(int32_t* width, int32_t* height);

    /**
     * \brief Writes the MGE compositor's current emulation front buffer into the destination buffer.
     * \param buffer The video buffer. Must be at least of size <c>width * height * 3</c>, as acquired by <c>mge_get_video_size</c>.
     */
    void copy_video(void* buffer);

    /**
     * \brief Draws the given data to the MGE surface
     * \param data The buffer holding video data. Must be at least of size <c>width * height * 3</c>, as acquired by <c>mge_get_video_size</c>.
     * \remarks The video buffer's size must match the current video size provided by <c>get_video_size</c>.
     */
    void load_screen(void* data);
} // namespace MGECompositor
