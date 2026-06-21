/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for providing helper functionality related to the Mupen64 core.
 */
namespace CoreUtils
{

/**
 * \brief Gets a user-friendly error message for a core result, if applicable.
 * \param result The result to get an error message for.
 * \return A pair of strings, where the first string is the error's module (e.g. "VCR") and the second string is the
 * error message. If no error message is applicable, both strings will be empty.
 */
std::pair<std::string, std::string> get_error_message_for_result(core_result result);

/**
 * Shows an error dialog for a core result. If the result indicates no error, no work is done.
 * \param result The result to show an error dialog for.
 * \param hwnd The parent window handle for the spawned dialog. If null, the main window is used.
 * \returns Whether the function was able to show an error dialog.
 */
bool show_error_dialog_for_result(core_result result, HWND hwnd = nullptr);

} // namespace CoreUtils
