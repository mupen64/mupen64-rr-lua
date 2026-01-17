/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * A module responsible for abstracting a settings-oriented ListView control.
 */
namespace SettingsListView
{
using t_group = std::pair<size_t, std::wstring>;
using t_item = std::pair<size_t, std::wstring>;

/**
 * \brief The context of a SettingsListView.
 */
struct t_settings_listview_context
{

    /**
     * \brief The ListView's parent dialog.
     */
    HWND dlg_hwnd;

    /**
     * \brief The ListView's client bounds.
     */
    RECT rect;

    /**
     * \brief A callback that is invoked when an item is about to be edited.
     */
    std::function<void(size_t index)> on_edit_start;

    /**
     * \brief The ListView's groups as a pair of group ID and group name.
     */
    std::vector<t_group> groups;

    /**
     * \brief The ListView's items as a pair of group ID and item name.
     */
    std::vector<t_item> items;

    /**
     * \brief A callback that retrieves an item's tooltip from the second column.
     */
    std::function<std::wstring(size_t index)> get_item_tooltip;

    /**
     * \brief A callback that retrieves an item's text from an arbitrary column.
     */
    std::function<std::wstring(size_t index, size_t column)> get_item_text;

    /**
     * \brief A callback that retrieves an item's image index.
     */
    std::function<int32_t(size_t index)> get_item_image;
};

/**
 * \brief Creates a SettingsListView.
 * \param ctx The context of the SettingsListView.
 * \return The handle of the created ListView.
 */
HWND create(const t_settings_listview_context &ctx);

/**
 * \brief Notifies the SettingsListView of a WM_NOTIFY message.
 * \param dlg_hwnd The ListView's parent dialog.
 * \param lvhwnd The ListView handle.
 * \param lparam The window procedure's LPARAM.
 * \param wparam The window procedure's WPARAM.
 * \return Whether the message was handled.
 */
bool notify(HWND dlg_hwnd, HWND lvhwnd, LPARAM lparam, WPARAM wparam);
} // namespace SettingsListView
