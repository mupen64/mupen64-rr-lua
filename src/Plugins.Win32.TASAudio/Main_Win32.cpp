#include "Main_Win32.hpp"
#include "Config.hpp"
#include "Config_Win32.hpp"
#include "IOUtils.h"
#include "Main.hpp"
#include <Views.Win32/ViewPlugin.h>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

#include <windows.h>
#include <winerror.h>
#include <winnt.h>
#include <winreg.h>

HINSTANCE g_dll_handle = nullptr;

static constexpr wchar_t CFG_SUBKEY[] = L"Software\\N64 Emulation\\DLL\\TAS Audio";
static constexpr wchar_t VALUE_CONFIG[] = L"Config";
static constexpr wchar_t VALUE_VERSION[] = L"Version";
static constexpr DWORD CUR_CONFIG_VERSION = 1;

BOOL __stdcall DllMain(HINSTANCE hmod, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_dll_handle = hmod;

        std::vector<wchar_t> dll_path_buf(MAX_PATH, L'\0');
        DWORD gmfn_rc = GetModuleFileName(hmod, dll_path_buf.data(), dll_path_buf.size());

        // If the buffer isn't long enough, double the buffer size until it fits
        while (gmfn_rc == dll_path_buf.size())
        {
            dll_path_buf.resize(dll_path_buf.size() * 2);
            gmfn_rc = GetModuleFileName(hmod, dll_path_buf.data(), dll_path_buf.size());
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
    MessageBox((HWND)hParent, msg, L"About", MB_ICONASTERISK);
}

EXPORT void CALL DllConfig(void *hParent)
{
    SDLAudio::Config cfg = win32_read_config();
    if (SDLAudio::win32_show_config((HWND)hParent, cfg))
    {
        if (g_ef) g_ef->log_info(L"Saving config...");
        win32_write_config(cfg);
        if (g_backend.has_value()) g_backend->merge_cfg_live(cfg);
    }
}

SDLAudio::Config win32_read_config()
{

    SDLAudio::Config cfg;
    HKEY key = 0;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, CFG_SUBKEY, 0, KEY_READ, &key) != ERROR_SUCCESS)
    {
        win32_write_config(cfg);
        return cfg;
    }

    DWORD type = 0;
    DWORD size = 0;
    LSTATUS res = ERROR_SUCCESS;

    // Versioning system
    DWORD version = 0;
    size = sizeof(DWORD);
    res = RegQueryValueEx(key, VALUE_VERSION, NULL, &type, (BYTE *)&version, &size);
    if (res != ERROR_SUCCESS || type != REG_DWORD)
    {
        RegCloseKey(key);
        win32_write_config(cfg);
        return cfg;
    }

    // Overwrite config with default if version is wrong
    if (version != CUR_CONFIG_VERSION)
    {
        RegCloseKey(key);
        win32_write_config(cfg);
        return cfg;
    }

    // Determine length of current config string
    size = 0;
    res = RegQueryValueExW(key, VALUE_CONFIG, NULL, &type, NULL, &size);
    if (res != ERROR_SUCCESS || type != REG_SZ)
    {
        cfg = SDLAudio::Config{};
        RegCloseKey(key);
        win32_write_config(cfg);
        return cfg;
    }

    // Read config UTF-16 JSON
    std::wstring cfg_wstr(size / sizeof(wchar_t), L'\0');
    res = RegQueryValueEx(key, VALUE_CONFIG, NULL, NULL, (BYTE *)cfg_wstr.data(), &size);
    if (res != ERROR_SUCCESS)
    {
        cfg = SDLAudio::Config{};
        RegCloseKey(key);
        win32_write_config(cfg);
        return cfg;
    }

    // convert to UTF-8 and read into config
    try
    {
        std::stringstream sstr(IOUtils::to_utf8_string(cfg_wstr));
        cfg.read_from(sstr);
    }
    catch (const std::exception &e)
    {
        cfg = SDLAudio::Config{};
        RegCloseKey(key);
        win32_write_config(cfg);
        return cfg;
    }

    RegCloseKey(key);
    return cfg;
}
void win32_write_config(const SDLAudio::Config &cfg)
{
    HKEY key = 0;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, CFG_SUBKEY, 0, NULL, 0, KEY_WRITE, NULL, &key, NULL) != ERROR_SUCCESS)
    {
        // silently fail, should we try to make an error dialog show up?
        return;
    }

    // Set version
    if (RegSetValueEx(key, VALUE_VERSION, 0, REG_DWORD, (const BYTE *)(&CUR_CONFIG_VERSION), sizeof(DWORD)) !=
        ERROR_SUCCESS)
    {
        RegCloseKey(key);
        return;
    }

    // convert data to UTF-16 JSON
    std::wstring cfg_wstr;
    {
        std::stringstream sstr;
        cfg.write_to(sstr);
        cfg_wstr = IOUtils::to_wide_string(sstr.str());
    }

    // write UTF-16 JSON
    if (RegSetValueEx(key, VALUE_CONFIG, 0, REG_SZ, (const BYTE *)cfg_wstr.c_str(),
                      (cfg_wstr.size() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
    {
        RegCloseKey(key);
        return;
    }

    RegCloseKey(key);
}
