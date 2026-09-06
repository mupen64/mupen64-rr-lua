/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <variant>
#include <functional>
#include <cstdint>
#include <string>
#include <format>
#include <Core/Plugin.hpp>
#include <Common.Views/Hotkey.hpp>
#include <Common/Assert.hpp>

/**
 * Represents a settings option.
 */
struct OptionItem
{
    enum class Type : uint8_t
    {
        Bool,
        Number,
        Enum,
        String,
        Hotkey,
        Folder,
    };

    typedef std::variant<int32_t, double, std::string, Hotkey> DataVariant;

    struct ReadableProp
    {
        std::function<DataVariant()> get;

        explicit ReadableProp(const std::function<DataVariant()> &get) { this->get = get; }
    };

    struct WritableProp : ReadableProp
    {
        std::function<void(const DataVariant &)> set;

        WritableProp(const std::function<DataVariant()> &get, const std::function<void(const DataVariant &)> &set)
            : ReadableProp(get)
        {
            this->set = set;
        }
    };

    /**
     * The option's backing data type.
     */
    Type type;

    /**
     * The group this option belongs to.
     */
    size_t group_id;

    /**
     * The option's display name.
     */
    std::string name;

    /**
     * The option's tooltip, or an empty string if no tooltip is set.
     */
    std::string tooltip;

    WritableProp current_value;

    ReadableProp initial_value = ReadableProp([] -> DataVariant {
        need(false, "Initial value not set for option");
        return DataVariant{};
    });

    ReadableProp default_value;

    std::vector<std::pair<std::string, int32_t>> possible_values;

    /**
     * Function which returns why an option is read-only, or std::nullopt if it is not read-only.
     */
    std::function<std::optional<std::string>()> get_readonly_reason = [] { return std::nullopt; };

    /**
     * Gets the name of the option item.
     */
    [[nodiscard]] std::string get_name() const;

    /**
     * Gets the value name for the current backing data, or a fallback name if no match is found.
     */
    [[nodiscard]] std::string get_value_name() const;

    /**
     * Resets the value of the option to the default value.
     */
    void reset_to_default() const;

    /**
     * \brief Gets neatly formatted information about the option.
     */
    std::string get_friendly_info() const;

    /**
     * \brief Prompts the user to edit the option value.
     * \param parent The parent window handle for any dialogs spawned by this method.
     * \return Whether the user confirmed the edit.
     */
    bool edit(M64RRSpec::WindowHandle parent);
};

/**
 * Represents a group of options in the settings.
 */
struct OptionGroup
{
    /**
     * The group's unique identifier.
     */
    size_t id;

    /**
     * The group's name.
     */
    std::string name;

    /**
     * \brief The options that belong to this group.
     */
    std::vector<OptionItem> items;
};

/**
 * \brief Provides utilities surrounding option items and groups.
 */
namespace OptionUtils
{
inline std::string to_str_default(const double value)
{
    return std::format("{:.15g}", value);
}

inline double get_number_value(const OptionItem::DataVariant &value)
{
    if (std::holds_alternative<int32_t>(value))
    {
        return static_cast<double>(std::get<int32_t>(value));
    }
    if (std::holds_alternative<double>(value))
    {
        return std::get<double>(value);
    }

    need(false, "Number option does not hold an int32_t or double value");
    return 0.0;
}

inline OptionItem::DataVariant parse_number_value(const std::string &text, const OptionItem::DataVariant &current)
{
    if (std::holds_alternative<int32_t>(current))
    {
        return std::stoi(text);
    }
    if (std::holds_alternative<double>(current))
    {
        return std::stod(text);
    }

    need(false, "Number option does not hold an int32_t or double value");
    return current;
}
} // namespace OptionUtils
