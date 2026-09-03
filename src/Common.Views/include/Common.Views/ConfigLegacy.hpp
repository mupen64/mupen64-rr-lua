/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <ini.h>
#include <Common.Views/Config.hpp>

namespace AppConfig::Legacy
{

/**
 * \brief Loads the config values from the legacy INI structure into g_config.
 */
void handle_config_ini(mINI::INIStructure &ini);

/**
 * \brief Migrates old values from the specified config to new ones if possible.
 */
void migrate_config_ini(Config &config, const mINI::INIStructure &ini);

} // namespace AppConfig::Legacy
