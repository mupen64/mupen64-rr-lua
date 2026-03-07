/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for implementing a text editing dialog.
 */
namespace TextEditDialog
{
/**
 * \brief Parameters for the text edit dialog.
 */
struct t_params
{
    /**
     * \brief The parent window of the dialog.
     */
    HWND parent_hwnd{};

    /**
     * \brief The initial text to display in the editbox.
     */
    std::wstring text{};

    /**
     * \brief The caption of the dialog.
     */
    std::wstring caption = L"Edit Text";

    /**
     * \brief If true, the editbox will be read-only.
     */
    bool readonly{};
};

/**
 * \brief Shows the about dialog with the specified text.
 * \param params The parameters for the text edit dialog.
 * \return The text if the user clicked OK, or std::nullopt if the user clicked Cancel.
 */
std::optional<std::wstring> show(const t_params &params);
} // namespace TextEditDialog
