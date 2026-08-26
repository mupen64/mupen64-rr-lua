/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Common.Views/OptionItem.hpp>
#include <Common.Views/ActionManager.hpp>

std::string OptionItem::get_name() const
{
    if (type == Type::Hotkey) return ActionManager::get_display_name(name, true);
    return name;
}

std::string OptionItem::get_value_name() const
{
    const auto value = current_value.get();

    switch (type)
    {
    case Type::Bool:
        return std::get<int32_t>(value) != 0 ? "On" : "Off";
    case Type::Number:
        return OptionUtils::to_str_default(OptionUtils::get_number_value(value));
    case Type::Enum: {
        const auto enum_value = std::get<int32_t>(value);

        for (const auto &pair : possible_values)
        {
            if (enum_value == pair.second)
            {
                return pair.first;
            }
        }

        return std::format("Unknown ({})", enum_value);
    }
    case Type::String:
        return std::get<std::string>(value);
    case Type::Hotkey:
        return std::get<Hotkey>(value).to_string();
    case Type::Folder:
        return std::get<std::string>(value);
    default:
        NEED(false, "Unhandled option type in OptionItem::get_value_name");
    }
    return "";
}

void OptionItem::reset_to_default() const
{
    current_value.set(default_value.get());
}

std::string OptionItem::get_friendly_info() const
{
    std::string str;

    if (get_readonly_reason().has_value())
    {
        str += std::format("⚠️ - {}\n\n", get_readonly_reason().value());
    }

    str += tooltip.empty() ? "(no further information available)" : tooltip;

    if (possible_values.empty())
    {
        return str;
    }

    str += "\r\n\r\n";
    for (const auto &pair : possible_values)
    {
        str += std::format("{} - {}", pair.second, pair.first);

        if (pair.second == std::get<int32_t>(current_value.get()))
        {
            str += " (default)";
        }

        str += "\r\n";
    }

    return str;
}
