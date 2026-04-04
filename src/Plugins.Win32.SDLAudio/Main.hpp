#pragma once
#include "Config.hpp"
#include "Views.Win32/ViewPlugin.h"
#include <VersionNameHelpers.h>

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME(L"SDL Audio", L"1.0.0")

extern core_plugin_extended_funcs *g_ef;
extern std::filesystem::path g_dll_path;

SDLAudio::Config read_config();
void write_config(const SDLAudio::Config& config);