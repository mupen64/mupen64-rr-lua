/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for implementing file and folder picking functionality.
 */
namespace FilePicker
{
    /**
     * \brief Shows a file selection dialog.
     * \param id The dialog's unique identifier, which is used for persisting the path.
     * \param hwnd The dialog owner's window handle, or NULL.
     * \param filter A collection of valid extension patterns separated by semicolons. Example: `*.png;*.jpg`.
     * \return The chosen file's path.
     */
    std::filesystem::path show_open_dialog(const std::wstring& id, HWND hwnd, const std::wstring& filter);

    /**
     * \brief Shows a file save dialog.
     * \param id The dialog's unique identifier, which is used for persisting the path.
     * \param hwnd The dialog owner's window handle, or NULL.
     * \param filter A collection of valid extension patterns separated by semicolons. Example: `*.png;*.jpg`.
     * \return The chosen file's path.
     */
    std::filesystem::path show_save_dialog(const std::wstring& id, HWND hwnd, const std::wstring& filter);

    /**
     * \brief Shows a folder selection dialog.
     * \param id The dialog's unique identifier, which is used for persisting the path.
     * \param hwnd The dialog owner's window handle, or NULL.
     * \return The chosen folder's path.
     */
    std::filesystem::path show_folder_dialog(const std::wstring& id, HWND hwnd);
} // namespace FilePicker
