#ifndef GLN64_H
#define GLN64_H

#include <CommonPCH.h>
#include <VersionNameHelpers.h>
#include <core_api.h>
#include <Views.Win32/ViewPlugin.h>

extern HWND hWnd;
extern HWND hStatusBar;
extern HWND hToolBar;
extern HINSTANCE hInstance;

extern void (*CheckInterrupts)(void);
extern std::filesystem::path screenDirectory;

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME(L"TASVideo", L"1.4.0")


#endif
