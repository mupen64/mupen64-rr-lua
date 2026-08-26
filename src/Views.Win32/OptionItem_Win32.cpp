/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Common.Views/OptionItem.hpp>
#include <HotkeyUtils.hpp>
#include <components/FilePicker.hpp>
#include <components/TextEditDialog.hpp>

bool OptionItem::edit(const M64RRSpec::WindowHandle parent)
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
            .text = OptionUtils::to_str_default(OptionUtils::get_number_value(value)),
            .caption = std::format("Edit value for {}", name)});
        if (!result.has_value())
        {
            break;
        }

        try
        {
            current_value.set(OptionUtils::parse_number_value(result.value(), value));
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
