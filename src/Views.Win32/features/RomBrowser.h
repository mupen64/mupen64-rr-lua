/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once


namespace RomBrowser
{
    /**
     * \brief Initializes the rombrowser
     */
    void init();

    /**
     * \brief Builds the rombrowser contents
     */
    void build();

    /**
     * \brief Notifies the rombrowser of a parent receiving the WM_NOTIFY message
     * \param lparam The lparam value associated with the current message processing pass
     */
    void notify(long lparam);

    /**
     * \brief Finds the first rom from the available ROM list which matches the predicate
     * \param predicate A predicate which determines if the rom matches
     * \return The rom's path, or an empty string if no rom was found
     */
    std::wstring find_available_rom(const std::function<bool(const core_rom_header&)>& predicate);
} // namespace RomBrowser
