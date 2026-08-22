/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <cstdint>

/*
 * \brief Provides a dialog for the user to select a microcode.
 */
namespace MicrocodeDialog
{
/*
 * \brief Shows the microcode dialog and returns the selected microcode.
 * \return The selected microcode.
 */
uint32_t show();
} // namespace MicrocodeDialog
