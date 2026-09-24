/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for implementing the movie inspector dialog.
 */
namespace MovieDialog
{
struct Result
{
    HWND hwnd;
    std::filesystem::path path;
    unsigned short start_flag;
    std::string author;
    std::string description;
};

/**
 * \brief Shows a movie inspector dialog.
 * \param readonly Whether the movie is being viewed in read-only mode.
 * \param on_confirm A callback invoked when the user confirms their choices. Returns whether the dialog should close.
 * \return The user's interaction result.
 */
Result show(bool readonly, const std::function<bool(const Result &)> &on_confirm = [](auto &...) { return true; });

} // namespace MovieDialog
