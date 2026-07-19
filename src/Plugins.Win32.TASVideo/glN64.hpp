#pragma once

#include <Views.Win32/ZilmarExtSpecPlugin.h>

struct TASVideoContext
{
    HINSTANCE hinst;
    HWND emu_hwnd;
    HWND statusbar_hwnd;
    void (*check_interrupts)(void);
    std::filesystem::path config_directory;
};

extern TASVideoContext g_tas_ctx;
extern ZilmarExtSpec::ExtendedFuncs *g_ef;

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME(L"TAS Video")
