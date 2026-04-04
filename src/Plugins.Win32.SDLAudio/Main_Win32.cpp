#include "Main_Win32.hpp"
#include "Config.hpp"
#include "Config_Win32.hpp"
#include "Main.hpp"
#include <Views.Win32/ViewPlugin.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <windows.h>

HINSTANCE g_dll_handle = nullptr;

BOOL __stdcall DllMain(HINSTANCE hmod, DWORD reason, LPVOID)
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

EXPORT void CALL DllConfig(void* hParent) {
    SDLAudio::Config cfg = read_config();
    if (SDLAudio::show_config_win32((HWND) hParent, cfg)) {
        if (g_ef)
            g_ef->log_info(L"Saving config...");
        write_config(cfg);
    }
}