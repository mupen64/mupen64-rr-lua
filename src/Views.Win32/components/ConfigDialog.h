/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for implementing configuration dialogs.
 */
namespace ConfigDialog
{
/**
 * Represents a settings option.
 */
struct t_options_item
{
    enum class Type
    {
        Bool,
        Number,
        Enum,
        String,
        Hotkey,
        Folder,
    };

    typedef std::variant<int32_t, std::wstring, Hotkey::t_hotkey> data_variant;

    struct t_readonly_property
    {
        std::function<data_variant()> get{};

        explicit t_readonly_property(const std::function<data_variant()> &get) { this->get = get; }
    };

    struct t_readwrite_property : t_readonly_property
    {
        std::function<void(const data_variant &)> set{};

        t_readwrite_property(const std::function<data_variant()> &get,
                             const std::function<void(const data_variant &)> &set)
            : t_readonly_property(get)
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
    std::wstring name{};

    /**
     * The option's tooltip, or an empty string if no tooltip is set.
     */
    std::wstring tooltip{};

    t_readwrite_property current_value;

    t_readonly_property initial_value = t_readonly_property([] -> data_variant {
        runtime_assert(false, L"Initial value not set for option");
        return data_variant{};
    });

    t_readonly_property default_value;

    std::vector<std::pair<std::wstring, int32_t>> possible_values = {};

    /**
     * Function which returns whether the option can be changed. Useful for values which shouldn't be changed during
     * emulation.
     */
    std::function<bool()> is_readonly = [] { return false; };

    /**
     * Gets the name of the option item.
     */
    [[nodiscard]] std::wstring get_name() const;

    /**
     * Gets the value name for the current backing data, or a fallback name if no match is found.
     */
    [[nodiscard]] std::wstring get_value_name() const;

    /**
     * Resets the value of the option to the default value.
     */
    void reset_to_default() const;

    /**
     * \brief Gets neatly formatted information about the option.
     */
    std::wstring get_friendly_info() const;

    /**
     * \brief Prompts the user to edit the option value.
     * \param hwnd The parent window handle for any dialogs spawned by this method.
     * \return Whether the user confirmed the edit.
     */
    bool edit(HWND hwnd);
};

/**
 * Represents a group of options in the settings.
 */
struct t_options_group
{
    /**
     * The group's unique identifier.
     */
    size_t id;

    /**
     * The group's name.
     */
    std::wstring name;

    /**
     * \brief The options that belong to this group.
     */
    std::vector<t_options_item> items{};
};

/**
 * \brief Shows the application settings dialog.
 */
void show_app_settings();

/**
 * \brief Gets all option groups.
 */
std::vector<t_options_group> get_option_groups();

} // namespace ConfigDialog
