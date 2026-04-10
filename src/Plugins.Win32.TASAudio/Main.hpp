#pragma once
#include "Config.hpp"
#include "SDLBackend.hpp"
#include "Views.Win32/ViewPlugin.h"
#include <VersionNameHelpers.h>

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME(L"TAS Audio", L"2.0.0")

extern core_plugin_extended_funcs *g_ef;
extern std::filesystem::path g_dll_path;
extern std::optional<SDLAudio::SDLBackend> g_backend;

SDLAudio::Config read_config();
void write_config(const SDLAudio::Config &config);