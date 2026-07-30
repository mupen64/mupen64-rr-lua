/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for wrapping file pickers with IDs for persisting paths across sessions.
 */
namespace FilePicker
{
/**
 * \brief Shows a file selection dialog. Wraps \ref WinFilePicker::show_open_dialog.
 */
std::filesystem::path show_open_dialog(const std::wstring &id, HWND hwnd, const std::wstring &filter);

/**
 * \brief Shows a file save dialog. Wraps \ref WinFilePicker::show_save_dialog.
 */
std::filesystem::path show_save_dialog(const std::wstring &id, HWND hwnd, const std::wstring &filter);

/**
 * \brief Shows a folder selection dialog. Wraps \ref WinFilePicker::show_folder_dialog.
 */
std::filesystem::path show_folder_dialog(const std::wstring &id, HWND hwnd);
} // namespace FilePicker
