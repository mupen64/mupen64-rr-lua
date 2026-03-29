#include "Main_Win32.hpp"
#include <Views.Win32/ViewPlugin.h>
#include <filesystem>
#include <libloaderapi.h>
#include <minwindef.h>
#include <string>
#include <vector>
#include <winnt.h>

HMODULE g_dll_handle = nullptr;
std::filesystem::path g_dll_path {};

BOOL __stdcall DllMain(HMODULE hmod, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_dll_handle = hmod;

        std::vector<wchar_t> dll_path_buf(MAX_PATH, L'\0');
        DWORD gmfn_rc = GetModuleFileNameW(hmod, dll_path_buf.data(), dll_path_buf.size());

        // If the buffer isn't long enough, double the buffer size until it fits
        while (gmfn_rc == dll_path_buf.size()) {
            dll_path_buf.resize(dll_path_buf.size() * 2);
            gmfn_rc = GetModuleFileNameW(hmod, dll_path_buf.data(), dll_path_buf.size());
        }

        // set the DLL path
        g_dll_path = std::filesystem::path(dll_path_buf.data());
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