#pragma once

struct TASVideoContext
{
    HWND statusbar_hwnd;
};

extern TASVideoContext g_tas_ctx;
extern HWND hWnd;
extern HINSTANCE hInstance;
extern void (*CheckInterrupts)(void);
extern std::filesystem::path screenDirectory;
extern core_plugin_extended_funcs *g_ef;

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME(L"TAS Video", L"1.4.1")
