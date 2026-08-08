/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <string>
#include <windows.h>
#include <Common.Views/Hotkey.hpp>

/**
 * \brief A module responsible for providing auxiliary functionality related to hotkeys.
 */
namespace HotkeyUtils
{
/**
 * \brief Shows a dialog prompting the user to enter a hotkey.
 * \param hwnd The parent window handle for the dialog.
 * \param caption The headline to display in the dialog.
 * \param hotkey The hotkey to set.
 * \return Whether the user confirmed the dialog. If the user cancelled the dialog, the hotkey won't have changed.
 */
bool show_prompt(HWND hwnd, const std::wstring &caption, Hotkey &hotkey);

/**
 * \brief Tries associating the specified action with the specified hotkey. Checks for a hotkey conflict and, if
 * necessary, prompts the user to fix the conflict. \param hwnd The parent window handle for the conflict dialog. \param
 * action The action to associate the hotkey with. \param new_hotkey The new hotkey to associate with the action. \param
 * through_action_manager Whether the ActionManager should be called to associate the hotkey. If false, the hotkey will
 * only be set in the config.
 */
void try_associate_hotkey(HWND hwnd, const std::wstring &action, const Hotkey &new_hotkey,
                          bool through_action_manager = true);

} // namespace HotkeyUtils
