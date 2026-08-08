/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Common.Views/ActionManager.hpp>

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
