/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <windows.h>

namespace TASVideo
{
/*
 * \brief Provides a config dialog.
 */
namespace ConfigDialog
{
/*
 * \brief Shows the config dialog.
 * \param parent The parent window.
 */
void show(HWND parent);
} // namespace ConfigDialog
} // namespace TASVideo
