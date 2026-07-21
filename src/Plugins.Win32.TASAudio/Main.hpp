/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once
#include "Config.hpp"
#include "SDLBackend.hpp"
#include "Views.Win32/ZilmarExtSpecPlugin.h"
#include <VersionNameHelpers.hpp>

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME(L"TAS Audio")

extern ZilmarExtSpec::ExtendedFuncs *g_ef;
extern std::filesystem::path g_dll_path;
extern std::filesystem::path g_config_path;
extern std::optional<SDLAudio::SDLBackend> g_backend;

SDLAudio::Config read_config();
void write_config(const SDLAudio::Config &config);
