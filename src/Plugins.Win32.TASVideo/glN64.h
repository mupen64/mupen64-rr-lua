#pragma once

extern HWND hWnd;
extern HWND hStatusBar;
extern HWND hToolBar;
extern HINSTANCE hInstance;

extern void (*CheckInterrupts)(void);
extern std::filesystem::path screenDirectory;

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME(L"TAS Video", L"1.4.0")
