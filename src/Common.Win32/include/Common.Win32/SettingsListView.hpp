/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <windows.h>
#include <string>
#include <cstdint>
#include <utility>

/**
 * A module responsible for abstracting a settings-oriented ListView control.
 */
namespace SettingsListView
{
using t_group = std::pair<size_t, std::string>;
using t_item = std::pair<size_t, std::string>;

/**
 * \brief The context of a SettingsListView.
 */
struct t_settings_listview_context
{
    /**
     * \brief The instance handle of the application.
     */
    HINSTANCE hinstance;

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
    std::function<std::string(size_t index)> get_item_tooltip;

    /**
     * \brief A callback that retrieves an item's text from an arbitrary column.
     */
    std::function<std::string(size_t index, size_t column)> get_item_text;

    /**
     * \brief A callback that retrieves an item's image index.
     */
    std::function<int32_t(size_t index)> get_item_image;
};

namespace detail
{
#define PROP_NAME "slv_ctx"

inline bool begin_listview_edit(SettingsListView::t_settings_listview_context *ctx, HWND lvhwnd)
{
    int32_t i = ListView_GetNextItem(lvhwnd, -1, LVNI_SELECTED);

    if (i == -1) return false;

    LVITEM item = {0};
    item.mask = LVIF_PARAM;
    item.iItem = i;
    ListView_GetItem(lvhwnd, &item);

    ctx->on_edit_start(item.lParam);

    ListView_Update(lvhwnd, i);
    return true;
}

inline LRESULT CALLBACK list_view_proc(
    HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param, UINT_PTR, DWORD_PTR ref_data)
{
    auto ctx = (SettingsListView::t_settings_listview_context *)ref_data;

    switch (msg)
    {
    case WM_KEYDOWN:
        if (w_param == VK_SPACE)
        {
            return TRUE;
        }
        break;
    case WM_KEYUP:
        if (w_param == VK_SPACE && begin_listview_edit(ctx, hwnd))
        {
            return TRUE;
        }
        break;
    case WM_NCDESTROY: {
        RemoveProp(ctx->dlg_hwnd, PROP_NAME);
        delete ctx;
        break;
    }
    default:
        break;
    }
    return DefSubclassProc(hwnd, msg, w_param, l_param);
}

inline HWND create_impl(const t_settings_listview_context &ctx)
{
    auto ctx2 = new t_settings_listview_context();
    *ctx2 = ctx;

    HWND lvhwnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL,
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | LVS_SINGLESEL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNTOP,
        ctx.rect.left, ctx.rect.top, ctx.rect.right - ctx.rect.left, ctx.rect.bottom - ctx.rect.top, ctx.dlg_hwnd,
        (HMENU)IDC_SETTINGS_LV, ctx.hinstance, NULL);

    SetProp(ctx.dlg_hwnd, PROP_NAME, ctx2);

    SetWindowSubclass(lvhwnd, list_view_proc, 0, (DWORD_PTR)ctx2);

    HIMAGELIST image_list = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 2, 0);
    ImageList_AddMaskedFromBitmap(image_list, ctx.hinstance, IDB_DENY);
    ImageList_AddMaskedFromBitmap(image_list, ctx.hinstance, IDB_CHANGED);
    ListView_SetImageList(lvhwnd, image_list, LVSIL_SMALL);

    ListView_EnableGroupView(lvhwnd, true);
    ListView_SetExtendedListViewStyle(
        lvhwnd, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_LABELTIP);

    LVGROUP lvgroup;
    lvgroup.cbSize = sizeof(LVGROUP);
    lvgroup.mask = LVGF_HEADER | LVGF_GROUPID;
    // TODO: Make groups collapsible
    // lvgroup.state = LVGS_COLLAPSIBLE;

    for (const auto &pair : ctx.groups)
    {
        const auto header = IOUtils::to_wide_string(pair.second);
        lvgroup.pszHeader = const_cast<LPWSTR>(header.c_str());
        lvgroup.iGroupId = pair.first;
        ListView_InsertGroup(lvhwnd, -1, &lvgroup);
    }

    LVCOLUMN lv_column = {0};
    lv_column.mask = LVCF_FMT | LVCF_DEFAULTWIDTH | LVCF_TEXT | LVCF_SUBITEM;

    lv_column.pszText = const_cast<LPSTR>("Name");
    ListView_InsertColumn(lvhwnd, 0, &lv_column);
    lv_column.pszText = const_cast<LPSTR>("Value");
    ListView_InsertColumn(lvhwnd, 1, &lv_column);

    LVITEM lv_item = {0};
    lv_item.mask = LVIF_TEXT | LVIF_GROUPID | LVIF_PARAM | LVIF_IMAGE;
    lv_item.pszText = LPSTR_TEXTCALLBACK;
    lv_item.iImage = I_IMAGECALLBACK;
    for (int i = 0; i < ctx.items.size(); ++i)
    {
        lv_item.iGroupId = ctx.items[i].first;
        lv_item.lParam = i;
        lv_item.iItem = i;
        ListView_InsertItem(lvhwnd, &lv_item);
    }

    // 1st column fits header
    ListView_SetColumnWidth(lvhwnd, 0, LVSCW_AUTOSIZE_USEHEADER);

    // 2nd column fills remaining space
    const auto first_column_width = ListView_GetColumnWidth(lvhwnd, 0);
    RECT lv_rc{};
    GetClientRect(lvhwnd, &lv_rc);
    ListView_SetColumnWidth(lvhwnd, 1, lv_rc.right - first_column_width);

    return lvhwnd;
}

inline bool notify_impl(HWND dlg_hwnd, HWND lvhwnd, LPARAM lparam, WPARAM wparam)
{
    if (wparam != IDC_SETTINGS_LV)
    {
        return false;
    }

    const auto lpnmhdr = reinterpret_cast<LPNMHDR>(lparam);
    auto ctx = (t_settings_listview_context *)GetProp(dlg_hwnd, PROP_NAME);

    switch (lpnmhdr->code)
    {
    case NM_DBLCLK:
        if (begin_listview_edit(ctx, lvhwnd))
        {
            return TRUE;
        }
        break;
    case LVN_GETDISPINFOA:
    case LVN_GETDISPINFOW: {
        auto fill = [&](auto *plvdi) {
            const auto i = plvdi->item.lParam;
            if (plvdi->item.mask & LVIF_IMAGE)
            {
                plvdi->item.iImage = ctx->get_item_image(i);
            }
            copy_listview_text(
                plvdi->item.pszText, plvdi->item.cchTextMax, ctx->get_item_text(i, plvdi->item.iSubItem));
        };

        if (lpnmhdr->code == LVN_GETDISPINFOA)
            fill(reinterpret_cast<NMLVDISPINFOA *>(lparam));
        else
            fill(reinterpret_cast<NMLVDISPINFOW *>(lparam));
        break;
    }
    case LVN_GETINFOTIPA:
    case LVN_GETINFOTIPW: {
        auto fill = [&](auto *getinfotip) {
            LVITEM item = {0};
            item.mask = LVIF_PARAM;
            item.iItem = getinfotip->iItem;
            ListView_GetItem(lvhwnd, &item);
            copy_listview_text(getinfotip->pszText, getinfotip->cchTextMax, ctx->get_item_tooltip(item.iItem));
        };

        if (lpnmhdr->code == LVN_GETINFOTIPA)
            fill(reinterpret_cast<LPNMLVGETINFOTIPA>(lparam));
        else
            fill(reinterpret_cast<LPNMLVGETINFOTIPW>(lparam));
        break;
    }
    default:
        return false;
    }

    return true;
}

} // namespace detail

/**
 * \brief Creates a SettingsListView.
 * \param ctx The context of the SettingsListView.
 * \return The handle of the created ListView.
 */
HWND create(const t_settings_listview_context &ctx)
{
    return detail::create_impl(ctx);
}

/**
 * \brief Notifies the SettingsListView of a WM_NOTIFY message.
 * \param dlg_hwnd The ListView's parent dialog.
 * \param lvhwnd The ListView handle.
 * \param lparam The window procedure's LPARAM.
 * \param wparam The window procedure's WPARAM.
 * \return Whether the message was handled.
 */
bool notify(HWND dlg_hwnd, HWND lvhwnd, LPARAM lparam, WPARAM wparam)
{
    return detail::notify_impl(dlg_hwnd, lvhwnd, lparam, wparam);
}

} // namespace SettingsListView
