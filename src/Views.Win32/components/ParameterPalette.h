/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <ActionManager.h>

/**
 * \brief A module responsible for implementing a parameter palette, which collects parameters for actions.
 */
namespace ParameterPalette
{
/**
 * \brief Shows the parameter palette.
 * \param action_path The action for which to collect parameters.
 */
void show(const ActionManager::action_path &action_path);

/**
 * \brief Gets the HWND of the parameter palette window. Might be invalid.
 */
HWND hwnd();
} // namespace ParameterPalette
