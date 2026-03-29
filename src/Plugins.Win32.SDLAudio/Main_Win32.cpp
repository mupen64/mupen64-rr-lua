#include "Main_Win32.hpp"
#include <Views.Win32/ViewPlugin.h>
#include <winnt.h>

HMODULE g_dll_handle = nullptr;

BOOL __stdcall DllMain(HMODULE hmod, DWORD reason, LPVOID)
{
  if (reason == DLL_PROCESS_ATTACH) {
    g_dll_handle = hmod;
  }
  return TRUE;
}
EXPORT void CALL DllAbout(void *hParent)
{
    const auto *msg = PLUGIN_NAME L"\n"
                                  L"Part of the Mupen64 project family."
                                  L"\n\n"
                                  L"https://github.com/mupen64/mupen64-rr-lua";
    MessageBoxW((HWND)hParent, msg, L"About", 0x00000040L | 0x00000000L);
}