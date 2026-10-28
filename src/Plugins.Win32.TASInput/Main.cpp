/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Main.hpp>
#include <TASInput.hpp>
#include <GamepadManager.hpp>
#include <ConfigDialog.hpp>

HINSTANCE g_inst;

namespace BuiltinTAS::TASInput
{
M64RRSpec::PluginInit *g_plugin;
}
