/***************************************************************************
                                                  rombrowser.h  -  description
                                                         -------------------
        copyright            : (C) 2003 by ShadowPrince
        email                : shadow@emulation64.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <core/r4300/rom.h>
#include <functional>
#include <string>
#include <vector>

namespace Rombrowser {
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
 * \param lparam The lparam value associated with the current message processing
 * pass
 */
void notify(long lparam);

/**
 * \brief Finds the first rom from the available ROM list which matches the
 * predicate \param predicate A predicate which determines if the rom matches
 * \return The rom's path, or an empty string if no rom was found
 */
std::wstring
find_available_rom(std::function<bool(const t_rom_header &)> predicate);
} // namespace Rombrowser
