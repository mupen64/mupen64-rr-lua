/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Common/Assert.hpp>
#include <Common.Views/OptionItem.hpp>

/**
 * \brief A module responsible for implementing configuration dialogs.
 */
namespace ConfigDialog
{

/**
 * \brief Shows the application settings dialog.
 */
void show_app_settings();

/**
 * \brief Gets all option groups.
 */
std::vector<t_options_group> get_option_groups();

} // namespace ConfigDialog
