/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for implementing statusbar functionality.
 */
namespace Statusbar
{
/**
 * \brief Represents different sections of the statusbar.
 */
enum class Section
{
    Notification,
    VCR,
    Readonly,
    Input,
    Rerecords,
    FPS,
    VIs,
    Slot,
    MultiFrameAdvanceCount,
    // Special marker, don't use
    Last,
};

/**
 * \brief Creates the statusbar.
 */
void create();

/**
 * \brief Gets the statusbar handle
 */
HWND hwnd();

/**
 * \brief Shows text in the statusbar.
 * \param text The text to be displayed.
 * \param section The statusbar section to display the text in.
 * \remark This function is thread-safe.
 */
void post(const std::wstring &text, Section section = Section::Notification);
} // namespace Statusbar
