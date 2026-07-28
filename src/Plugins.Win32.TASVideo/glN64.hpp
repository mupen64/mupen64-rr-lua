#pragma once

#include <m64rr/Plugin.hpp>

struct TASVideoContext
{
    HINSTANCE hinst;
};

extern TASVideoContext g_tas_ctx;
extern M64RRSpec::PluginInit *g_plugin;

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME(L"TAS Video")
