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
    [[nodiscard]] bool is_nothing() const;
    [[nodiscard]] std::wstring to_wstring() const;
};
