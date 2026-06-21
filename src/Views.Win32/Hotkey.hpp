/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
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
struct t_hotkey
{
    int32_t key{};
    bool ctrl{};
    bool shift{};
    bool alt{};
    bool assigned{};

    explicit t_hotkey(const int32_t key, const bool ctrl = false, const bool shift = false, const bool alt = false)
        : key(key), ctrl(ctrl), shift(shift), alt(alt), assigned(true)
    {
    }

    t_hotkey() = default;

    /**
     * \brief Gets whether the hotkey is empty. This is different to having no assignment, as it means an intentional
     * user override.
     */
    [[nodiscard]] bool is_empty() const;

    /**
     * \brief Gets whether the hotkey has no assignment.
     */
    [[nodiscard]] bool is_assigned() const;

    /**
     * \brief Gets the string representation of the hotkey.
     */
    [[nodiscard]] std::wstring to_wstring() const;

    /**
     * \returns An empty hotkey.
     */
    [[nodiscard]] static t_hotkey make_empty();

    /**
     * \returns An unassigned hotkey.
     */
    [[nodiscard]] static t_hotkey make_unassigned();

    bool operator==(const t_hotkey &other) const
    {
        return key == other.key && ctrl == other.ctrl && shift == other.shift && alt == other.alt &&
               assigned == other.assigned;
    }
};

/**
 * \brief Shows a dialog prompting the user to enter a hotkey.
 * \param hwnd The parent window handle for the dialog.
 * \param caption The headline to display in the dialog.
 * \param hotkey The hotkey to set.
 * \return Whether the user confirmed the dialog. If the user cancelled the dialog, the hotkey won't have changed.
 */
bool show_prompt(HWND hwnd, const std::wstring &caption, t_hotkey &hotkey);

/**
 * \brief Tries associating the specified action with the specified hotkey. Checks for a hotkey conflict and, if
 * necessary, prompts the user to fix the conflict. \param hwnd The parent window handle for the conflict dialog. \param
 * action The action to associate the hotkey with. \param new_hotkey The new hotkey to associate with the action. \param
 * through_action_manager Whether the ActionManager should be called to associate the hotkey. If false, the hotkey will
 * only be set in the config.
 */
void try_associate_hotkey(HWND hwnd, const std::wstring &action, const t_hotkey &new_hotkey,
                          bool through_action_manager = true);

} // namespace Hotkey
