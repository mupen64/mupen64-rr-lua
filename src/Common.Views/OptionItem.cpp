/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Common.Views/OptionItem.hpp>
#include <Common/Assert.hpp>

static std::string to_str_default(const double value)
{
    return std::format("{:.15g}", value);
}

static double get_number_value(const t_options_item::data_variant &value)
{
    if (std::holds_alternative<int32_t>(value))
    {
        return static_cast<double>(std::get<int32_t>(value));
    }
    if (std::holds_alternative<double>(value))
    {
        return std::get<double>(value);
    }

    NEED(false, "Number option does not hold an int32_t or double value");
    return 0.0;
}

static t_options_item::data_variant parse_number_value(
    const std::string &text, const t_options_item::data_variant &current)
{
    if (std::holds_alternative<int32_t>(current))
    {
        return std::stoi(text);
    }
    if (std::holds_alternative<double>(current))
    {
        return std::stod(text);
    }

    NEED(false, "Number option does not hold an int32_t or double value");
    return current;
}

std::string t_options_item::get_name() const
{
    if (type == Type::Hotkey) return ActionManager::get_display_name(name, true);
    return name;
}

std::string t_options_item::get_value_name() const
{
    const auto value = current_value.get();

    switch (type)
    {
    case Type::Bool:
        return std::get<int32_t>(value) != 0 ? "On" : "Off";
    case Type::Number:
        return to_str_default(get_number_value(value));
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
        NEED(false, "Unhandled option type in t_options_item::get_value_name");
    }
    return "";
}

void t_options_item::reset_to_default() const
{
    current_value.set(default_value.get());
}

std::string t_options_item::get_friendly_info() const
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

bool t_options_item::edit(const M64RRSpec::WindowHandle parent)
{
    const auto hwnd = parent.hwnd();

    switch (type)
    {
    case Type::Bool: {
        const auto new_value = std::get<int32_t>(current_value.get()) == 0 ? 1 : 0;
        current_value.set(new_value);
        return true;
    }
    case Type::Number: {
        const auto value = current_value.get();
        const auto result = TextEditDialog::show({.parent_hwnd = hwnd,
            .text = to_str_default(get_number_value(value)),
            .caption = std::format("Edit value for {}", name)});
        if (!result.has_value())
        {
            break;
        }

        try
        {
            current_value.set(parse_number_value(result.value(), value));
            return true;
        }
        catch (...)
        {
        }
        break;
    }
    case Type::Enum: {
        // 1. Find the index of the currently selected item, while falling back to the first possible value if there's
        // no match
        int32_t val = possible_values[0].second;
        for (const auto &[_, possible_value] : possible_values)
        {
            if (std::get<int32_t>(current_value.get()) == possible_value)
            {
                val = possible_value;
                break;
            }
        }

        // 2. Find the lowest and highest values in the vector
        int32_t min_possible_value = INT32_MAX;
        int32_t max_possible_value = INT32_MIN;
        for (const auto &val : possible_values | std::views::values)
        {
            max_possible_value = std::max(val, max_possible_value);
            min_possible_value = std::min(val, min_possible_value);
        }

        // 2. Bump it, wrapping around if needed
        val++;
        if (val > max_possible_value)
        {
            val = min_possible_value;
        }

        // 3. Apply the change
        current_value.set(val);
        return true;
    }
    case Type::String: {
        const auto value = std::get<std::string>(current_value.get());
        const auto result = TextEditDialog::show(
            {.parent_hwnd = hwnd, .text = value, .caption = std::format("Edit value for {}", name)});
        if (result.has_value())
        {
            current_value.set(result.value());
            return true;
        }
        break;
    }
    case Type::Hotkey: {
        auto hotkey = std::get<Hotkey>(current_value.get());
        HotkeyUtils::show_prompt(hwnd, std::format("Choose a hotkey for {}", name), hotkey);
        HotkeyUtils::try_associate_hotkey(hwnd, name, hotkey, false);
        return true;
    }
    case Type::Folder: {
        const auto path = FilePicker::show_folder_dialog(this->name, hwnd);
        if (!path.empty())
        {
            current_value.set(path.string());
            return true;
        }
        break;
    }
    default:
        break;
    }

    return false;
}
