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
 * Shows an error dialog for a core result. If the result indicates no error, no work is done.
 * \param result The result to show an error dialog for.
 * \param hwnd The parent window handle for the spawned dialog. If null, the main window is used.
 * \returns Whether the function was able to show an error dialog.
 */
bool show_error_dialog_for_result(core_result result, HWND hwnd = nullptr);

} // namespace CoreUtils
