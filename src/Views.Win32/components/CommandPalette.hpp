/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for implementing a command palette.
 */
namespace CommandPalette
{
/**
 * \brief Shows the command palette.
 */
void show();

/**
 * \brief Gets the HWND of the command palette window. Might be invalid.
 */
HWND hwnd();

/**
 * \brief Gets the recommended screen-space bounds for a palette window.
 * \param preferred_height The preferred height of the window. If not specified, a default height is calculated.
 * \return The recommended bounds.
 */
RECT get_recommended_bounds(std::optional<int32_t> preferred_height = std::nullopt);
} // namespace CommandPalette
