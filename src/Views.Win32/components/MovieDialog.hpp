/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for implementing the movie inspector dialog.
 */
namespace MovieDialog
{
struct t_result
{
    HWND hwnd;
    std::filesystem::path path;
    unsigned short start_flag;
    std::wstring author;
    std::wstring description;
    int32_t pause_at;
    int32_t pause_at_last;
};

/**
 * \brief Shows a movie inspector dialog.
 * \param readonly Whether the movie is being viewed in read-only mode.
 * \param on_confirm A callback invoked when the user confirms their choices. Returns whether the dialog should close.
 * \return The user's interaction result.
 */
t_result show(bool readonly, const std::function<bool(const t_result &)> &on_confirm = [](auto &...) { return true; });

} // namespace MovieDialog
