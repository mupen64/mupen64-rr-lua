/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <string>
#include <Common.Win32/Common.hpp>
#include <Common.Views/Hotkey.hpp>

/**
 * \brief A module responsible for providing auxiliary functionality related to hotkeys.
 */
namespace HotkeyUtils
{
/**
 * \brief Tries to convert a Windows virtual key code to a `Hotkey::Trigger`.
 * \return The converted trigger, or `std::nullopt` if there is no equivalent.
 */
std::optional<::Hotkey::Trigger> vk_to_trigger(uint32_t vk);

/**
 * \brief Converts a Windows virtual key code to an SDL keycode.
 */
std::optional<SDL_Keycode> vk_to_keycode(uint32_t vk);

/**
 * \brief Converts a Windows keyboard message to an SDL keycode.
 * \param vk The virtual key code from the message's `wParam`.
 * \param key_data The key data from the message's `lParam`.
 */
std::optional<SDL_Keycode> message_to_keycode(uint32_t vk, LPARAM key_data);

/**
 * \brief Converts an SDL keycode to a Windows virtual key code.
 */
std::optional<uint32_t> keycode_to_vk(SDL_Keycode keycode);

/**
 * \brief Tries to convert a `Hotkey::Trigger` to a Windows virtual key code.
 * \return The converted virtual key code, or `std::nullopt` if there is no equivalent.
 */
std::optional<uint32_t> trigger_to_vk(const ::Hotkey::Trigger &trigger);

/**
 * \brief Shows a dialog prompting the user to enter a hotkey.
 * \param hwnd The parent window handle for the dialog.
 * \param caption The headline to display in the dialog.
 * \param hotkey The hotkey to set.
 * \return Whether the user confirmed the dialog. If the user cancelled the dialog, the hotkey won't have changed.
 */
bool show_prompt(HWND hwnd, const std::string &caption, ::Hotkey &hotkey);

/**
 * \brief Tries associating the specified action with the specified hotkey. Checks for a hotkey conflict and, if
 * necessary, prompts the user to fix the conflict. \param hwnd The parent window handle for the conflict dialog. \param
 * action The action to associate the hotkey with. \param new_hotkey The new hotkey to associate with the action. \param
 * through_action_manager Whether the ActionManager should be called to associate the hotkey. If false, the hotkey will
 * only be set in the config.
 */
void try_associate_hotkey(
    HWND hwnd, const std::string &action, const ::Hotkey &new_hotkey, bool through_action_manager = true);

} // namespace HotkeyUtils
