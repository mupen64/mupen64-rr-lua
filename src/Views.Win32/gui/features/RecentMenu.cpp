/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "RecentMenu.h"
#include <gui/Main.h>

void RecentMenu::build(const std::vector<std::wstring>& vec, const int first_menu_id, const HMENU parent_menu)
{
    assert(is_on_gui_thread());

    // First 3 items aren't dynamic and shouldn't be deleted
    constexpr int FIXED_ITEM_COUNT = 3;

    const int child_count = GetMenuItemCount(parent_menu) - FIXED_ITEM_COUNT;

    for (auto i = 0; i < child_count; ++i)
    {
        DeleteMenu(parent_menu, FIXED_ITEM_COUNT, MF_BYPOSITION);
    }

    MENUITEMINFO menu_info = {0};
    menu_info.cbSize = sizeof(MENUITEMINFO);
    menu_info.fMask = MIIM_TYPE | MIIM_ID;
    menu_info.fType = MFT_STRING;
    menu_info.fState = MFS_ENABLED;

    for (int i = vec.size() - 1; i >= 0; --i)
    {
        if (vec[i].empty())
        {
            continue;
        }
        // FIXME: This is concerning, but seems to work
        menu_info.dwTypeData = const_cast<LPTSTR>(vec[i].data());
        menu_info.cch = vec[i].size();
        menu_info.wID = first_menu_id + i;
        auto result = InsertMenuItem(parent_menu, FIXED_ITEM_COUNT, TRUE, &menu_info);
    }
}

void RecentMenu::add(std::vector<std::wstring>& vec, std::wstring val, const bool frozen, const int first_menu_id, const HMENU parent_menu)
{
    assert(is_on_gui_thread());

    if (frozen)
    {
        return;
    }
    if (vec.size() > 5)
    {
        vec.pop_back();
    }
    std::erase_if(vec, [&](const auto str) {
        return ::iequals(str, val) || std::filesystem::path(str).compare(val) == 0;
    });
    vec.insert(vec.begin(), val);
    build(vec, first_menu_id, parent_menu);
}

std::wstring RecentMenu::element_at(std::vector<std::wstring> vec, const int first_menu_id, const int menu_id)
{
    const int index = menu_id - first_menu_id;
    if (index >= 0 && index < vec.size())
    {
        return vec[index];
    }
    return L"";
}
