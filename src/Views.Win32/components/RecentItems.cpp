/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <components/RecentItems.hpp>
#include <Common.Views/ActionManager.hpp>
#include <components/RomBrowser.hpp>

void RecentMenu::add(
    const ActionManager::action_filter &menu_path, std::vector<std::string> &vec, std::string val, const bool frozen)
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
    std::erase_if(vec, [&](const auto &str) {
        return MiscHelpers::iequals(str, val) || std::filesystem::path(str).compare(val) == 0;
    });
    vec.insert(vec.begin(), val);

    ActionManager::notify_display_name_changed(std::format("{} > *", menu_path));
    RomBrowser::build();
}
