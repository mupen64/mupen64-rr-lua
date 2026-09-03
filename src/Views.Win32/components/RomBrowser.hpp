/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Core/Types.hpp>

/**
 * \brief A module responsible for implementing the rombrowser.
 */
namespace RomBrowser
{
struct SimpleROMInfo
{
    std::string path;
    size_t size;
    CoreROMHeader header;
};

/**
 * \brief Creates the rombrowser.
 */
void create();

/**
 * \brief Builds the rombrowser contents
 */
void build();

/**
 * \brief Notifies the rombrowser of a parent receiving the WM_NOTIFY message
 * \param lparam The lparam value associated with the current message processing pass
 */
LRESULT notify(LPARAM lparam);

/**
 * \brief Finds the first rom from the available ROM list which matches the predicate
 * \param predicate A predicate which determines if the rom matches
 * \return The rom's path, or an empty string if no rom was found
 */
std::filesystem::path find_available_rom(const std::function<bool(const CoreROMHeader &)> &predicate);

/**
 * \brief Finds ROMs from the available ROM list which match the predicate
 * \param predicate A predicate which determines if the rom matches
 * \return The rom paths.
 */
std::vector<std::filesystem::path> find_available_roms(const std::function<bool(const CoreROMHeader &)> &predicate);

/**
 * \brief Gets the list of discovered ROMs. This list is cached and updated when the rombrowser is built.
 * \return The discovered ROMs.
 */
std::vector<SimpleROMInfo> get_discovered_roms();

} // namespace RomBrowser
