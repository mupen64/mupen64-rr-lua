/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once
#include "Config.hpp"
#include "SDLBackend.hpp"
#include <Core/Plugin.hpp>
#include <Common/VersionNameHelpers.hpp>

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME("TAS Audio")

namespace BuiltinTAS::TASAudio
{
extern M64RRSpec::PluginInit *g_plugin;
extern std::optional<SDLAudio::SDLBackend> g_backend;
} // namespace BuiltinTAS::TASAudio

SDLAudio::Config read_config();
void write_config(const SDLAudio::Config &config);
