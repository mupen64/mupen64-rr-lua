/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <cstdint>
#include <string>

/**
 * \brief Represents a combination of a key and modifiers.
 */
struct Hotkey
{
    int32_t key{};
    bool ctrl{};
    bool shift{};
    bool alt{};
    bool assigned{};

    explicit Hotkey(const int32_t key, const bool ctrl = false, const bool shift = false, const bool alt = false)
        : key(key), ctrl(ctrl), shift(shift), alt(alt), assigned(true)
    {
    }

    Hotkey() = default;

    /**
     * \brief Gets whether the hotkey is empty. This is different to having no assignment, as it means an intentional
     * user override.
     */
    [[nodiscard]] bool is_empty() const;

    /**
     * \brief Gets whether the hotkey has no assignment.
     */
    [[nodiscard]] bool is_assigned() const;

    /**
     * \brief Gets the string representation of the hotkey.
     */
    [[nodiscard]] std::wstring to_wstring() const;

    /**
     * \returns An empty hotkey.
     */
    [[nodiscard]] static Hotkey make_empty();

    /**
     * \returns An unassigned hotkey.
     */
    [[nodiscard]] static Hotkey make_unassigned();

    bool operator==(const Hotkey &other) const
    {
        return key == other.key && ctrl == other.ctrl && shift == other.shift && alt == other.alt &&
               assigned == other.assigned;
    }
};
