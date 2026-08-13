/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <ini.h>
#include <Common.Views/Config.hpp>

constexpr auto FLAT_FIELD_KEY = "config";

/**
 * \brief Returns a config populated with default values.
 */
t_config get_default_config();

namespace Config::Legacy
{

/**
 * \brief Loads the config values from the legacy INI structure into g_config.
 */
void handle_config_ini(mINI::INIStructure &ini);

/**
 * \brief Migrates old values from the specified config to new ones if possible.
 */
void migrate_config_ini(t_config &config, const mINI::INIStructure &ini);

} // namespace Config::Legacy