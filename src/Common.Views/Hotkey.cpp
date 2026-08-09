/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Common.Views/Hotkey.hpp>
#include <IOUtils.hpp>
#include <SDL3/SDL_keyboard.h>

static constexpr std::string mbf_to_string(const SDL_MouseButtonFlags button)
{
    switch (button)
    {
    case SDL_BUTTON_LEFT:
        return "LMB";
    case SDL_BUTTON_RIGHT:
        return "RMB";
    case SDL_BUTTON_MIDDLE:
        return "MMB";
    case SDL_BUTTON_X1:
        return "MX1";
    case SDL_BUTTON_X2:
        return "MX2";
    default:
        return "";
    }
}

std::string Hotkey::to_string() const
{
    if (!is_assigned()) return "";
    if (is_empty()) return "(nothing)";

    std::string str;
    if (this->ctrl) str += "Ctrl ";
    if (this->shift) str += "Shift ";
    if (this->alt) str += "Alt ";
    if (std::holds_alternative<KeyCode>(trigger))
        str += SDL_GetKeyName(std::get<KeyCode>(trigger).get());
    else if (std::holds_alternative<MouseButton>(trigger))
        str += mbf_to_string(std::get<MouseButton>(trigger).get());

    return str;
}

std::wstring Hotkey::to_wstring() const
{
    return IOUtils::to_wide_string(this->to_string());
}
