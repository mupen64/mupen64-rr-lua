#pragma once

struct TASVideoContext
{
    HINSTANCE hinst;
    HWND emu_hwnd;
    HWND statusbar_hwnd;
    void (*check_interrupts)(void);
    std::filesystem::path screenshot_directory;
};

extern TASVideoContext g_tas_ctx;
extern core_plugin_extended_funcs *g_ef;

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME(L"TAS Video", L"1.4.1")
