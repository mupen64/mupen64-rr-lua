/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for providing the hotkey structure and related functionality.
 */
namespace Hotkey
{
    /**
     * \brief Represents a combination of a key and modifiers.
     */
    struct t_hotkey {
        int32_t key{};
        bool ctrl{};
        bool shift{};
        bool alt{};

        /**
         * \brief Gets whether the hotkey has no key or modifier set.
         */
        [[nodiscard]] bool is_nothing() const;

        /**
         * \brief Gets the string representation of the hotkey.
         */
        [[nodiscard]] std::wstring to_wstring() const;

        bool operator==(const t_hotkey& other) const
        {
            return key == other.key && ctrl == other.ctrl && shift == other.shift && alt == other.alt;
        }
    };

    /**
     * \brief Shows a dialog prompting the user to enter a hotkey.
     * \param hwnd The parent window handle for the dialog.
     * \param caption The headline to display in the dialog.
     * \param hotkey The hotkey to set.
     * \return Whether the user confirmed the dialog. If the user cancelled the dialog, the hotkey won't have changed.
     */
    bool show_prompt(HWND hwnd, const std::wstring& caption, t_hotkey& hotkey);
} // namespace Hotkey
