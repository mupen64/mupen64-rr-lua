/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <components/RecentMenu.h>
#include <Messenger.h>

void RecentMenu::add(std::vector<std::wstring>& vec, std::wstring val, const bool frozen)
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
        return io_service.iequals(str, val) || std::filesystem::path(str).compare(val) == 0;
    });
    vec.insert(vec.begin(), val);

    Messenger::broadcast(Messenger::Message::ActionRegistryChanged, nullptr);
}
