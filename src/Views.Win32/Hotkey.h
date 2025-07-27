/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief Represents a combination of key + modifier combination.
 */
struct t_hotkey {
    int32_t key{};
    bool ctrl{};
    bool shift{};
    bool alt{};

    /**
     * \brief Gets whether the hotkey has no key or modifier set.
     */
    [[nodiscard]] bool is_nothing() const;

    /**
     * \brief Gets the string representation of the hotkey.
     */
    [[nodiscard]] std::wstring to_wstring() const;

    bool operator==(const t_hotkey& other) const
    {
        return key == other.key && ctrl == other.ctrl && shift == other.shift && alt == other.alt;
    }
};
