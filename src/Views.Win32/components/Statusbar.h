/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for implementing statusbar functionality.
 */
namespace Statusbar
{
    enum class Section {
        Notification,
        VCR,
        Readonly,
        Input,
        Rerecords,
        FPS,
        VIs,
        Slot,
        MultiFrameAdvanceCount,
    };

    /**
     * \brief Creates the statusbar.
     */
    void create();

    /**
     * \brief Shows text in the statusbar
     * \param text The text to be displayed
     * \param section The statusbar section to display the text in
     */
    void post(const std::wstring& text, Section section = Section::Notification);

    /**
     * \brief Gets the statusbar handle
     */
    HWND hwnd();
} // namespace Statusbar
