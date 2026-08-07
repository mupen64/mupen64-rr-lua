/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Hotkey.hpp>
#include <IOUtils.hpp>
#include <SDL3/SDL_keyboard.h>

bool Hotkey::is_empty() const
{
    return !this->ctrl && !this->shift && !this->alt && this->key == 0;
}

bool Hotkey::is_assigned() const
{
    return this->assigned;
}

std::string Hotkey::to_string() const
{
    if (is_empty()) return "(nothing)";

    std::string str;
    if (this->ctrl) str += "Ctrl ";
    if (this->shift) str += "Shift ";
    if (this->alt) str += "Alt ";
    if (this->key) str += SDL_GetKeyName(this->key);

    return str;
}

std::wstring Hotkey::to_wstring() const
{
    return IOUtils::to_wide_string(this->to_string());
}

Hotkey Hotkey::make_empty()
{
    Hotkey hotkey;
    hotkey.assigned = true;
    return hotkey;
}

Hotkey Hotkey::make_unassigned()
{
    return {};
}
