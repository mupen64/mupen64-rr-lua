/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <components/RecentItems.h>
#include <ActionManager.h>

void RecentMenu::add(const ActionManager::action_filter& menu_path, std::vector<std::wstring>& vec, std::wstring val, const bool frozen)
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
    std::erase_if(vec, [&](const auto& str) {
        return io_service.iequals(str, val) || std::filesystem::path(str).compare(val) == 0;
    });
    vec.insert(vec.begin(), val);

    ActionManager::notify_display_name_changed(std::format(L"{} > *", menu_path));
}
