/*
 * Copyright (c) 2026, Aurumaker72.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * WinDarkMode 0.0.1
 * https://github.com/Aurumaker72/WinDarkMode
 * Single-header Win32 dark mode library with a sane and modern API surface.
 * Based on https://github.com/ysc3839/win32-darkmode and
 * https://github.com/stevemk14ebr/PolyHook_2_0/blob/master/sources/IatHook.cpp.
 */

/*
 *
 * Based on the MIT-licensed work by Richard Yu:
 *
 * MIT License
 *
 * Copyright (c) 2019 Richard Yu
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <Windows.h>
#include <Uxtheme.h>
#include <commdlg.h>
#include <Vssym32.h>
#include <dwmapi.h>
#include <winerror.h>
#include <cstdint>
#include <string>

#ifndef _UNICODE
#error WinDarkMode requires a Unicode build.
#endif

/**
 * @brief Namespace containing functions for enabling and using dark mode on Win32.
 */
namespace WinDarkMode
{

/**
 * @brief Internal stuff. Don't reference these.
 */
namespace Internal
{

enum IMMERSIVE_HC_CACHE_MODE
{
    IHCM_USE_CACHED_VALUE,
    IHCM_REFRESH
};

enum PreferredAppMode
{
    Default,
    AllowDark,
    ForceDark,
    ForceLight,
    Max
};

enum WINDOWCOMPOSITIONATTRIB
{
    WCA_UNDEFINED = 0,
    WCA_NCRENDERING_ENABLED = 1,
    WCA_NCRENDERING_POLICY = 2,
    WCA_TRANSITIONS_FORCEDISABLED = 3,
    WCA_ALLOW_NCPAINT = 4,
    WCA_CAPTION_BUTTON_BOUNDS = 5,
    WCA_NONCLIENT_RTL_LAYOUT = 6,
    WCA_FORCE_ICONIC_REPRESENTATION = 7,
    WCA_EXTENDED_FRAME_BOUNDS = 8,
    WCA_HAS_ICONIC_BITMAP = 9,
    WCA_THEME_ATTRIBUTES = 10,
    WCA_NCRENDERING_EXILED = 11,
    WCA_NCADORNMENTINFO = 12,
    WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
    WCA_VIDEO_OVERLAY_ACTIVE = 14,
    WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
    WCA_DISALLOW_PEEK = 16,
    WCA_CLOAK = 17,
    WCA_CLOAKED = 18,
    WCA_ACCENT_POLICY = 19,
    WCA_FREEZE_REPRESENTATION = 20,
    WCA_EVER_UNCLOAKED = 21,
    WCA_VISUAL_OWNER = 22,
    WCA_HOLOGRAPHIC = 23,
    WCA_EXCLUDED_FROM_DDA = 24,
    WCA_PASSIVEUPDATEMODE = 25,
    WCA_USEDARKMODECOLORS = 26,
    WCA_LAST = 27
};

struct WINDOWCOMPOSITIONATTRIBDATA
{
    WINDOWCOMPOSITIONATTRIB Attrib;
    PVOID pvData;
    SIZE_T cbData;
};

struct SubclassInfo
{
    COLORREF headerTextColor;
};

using fnRtlGetNtVersionNumbers = void(WINAPI *)(LPDWORD major, LPDWORD minor, LPDWORD build);
using fnSetWindowCompositionAttribute = BOOL(WINAPI *)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA *);
// 1809 17763
using fnShouldAppsUseDarkMode = bool(WINAPI *)();                                            // ordinal 132
using fnAllowDarkModeForWindow = bool(WINAPI *)(HWND hWnd, bool allow);                      // ordinal 133
using fnAllowDarkModeForApp = bool(WINAPI *)(bool allow);                                    // ordinal 135, in 1809
using fnFlushMenuThemes = void(WINAPI *)();                                                  // ordinal 136
using fnRefreshImmersiveColorPolicyState = void(WINAPI *)();                                 // ordinal 104
using fnIsDarkModeAllowedForWindow = bool(WINAPI *)(HWND hWnd);                              // ordinal 137
using fnGetIsImmersiveColorUsingHighContrast = bool(WINAPI *)(IMMERSIVE_HC_CACHE_MODE mode); // ordinal 106
using fnOpenNcThemeData = HTHEME(WINAPI *)(HWND hWnd, LPCWSTR pszClassList);                 // ordinal 49
// 1903 18362
using fnShouldSystemUseDarkMode = bool(WINAPI *)();                                 // ordinal 138
using fnSetPreferredAppMode = PreferredAppMode(WINAPI *)(PreferredAppMode appMode); // ordinal 135, in 1903
using fnIsDarkModeAllowedForApp = bool(WINAPI *)();                                 // ordinal 139

constexpr COLORREF bg_color = 0x383838;
constexpr COLORREF text_color = 0xFFFFFF;

inline fnSetWindowCompositionAttribute _SetWindowCompositionAttribute{};
inline fnShouldAppsUseDarkMode _ShouldAppsUseDarkMode{};
inline fnAllowDarkModeForWindow _AllowDarkModeForWindow{};
inline fnAllowDarkModeForApp _AllowDarkModeForApp{};
inline fnFlushMenuThemes _FlushMenuThemes{};
inline fnRefreshImmersiveColorPolicyState _RefreshImmersiveColorPolicyState{};
inline fnIsDarkModeAllowedForWindow _IsDarkModeAllowedForWindow{};
inline fnGetIsImmersiveColorUsingHighContrast _GetIsImmersiveColorUsingHighContrast{};
inline fnOpenNcThemeData _OpenNcThemeData{};
inline fnShouldSystemUseDarkMode _ShouldSystemUseDarkMode{};
inline fnSetPreferredAppMode _SetPreferredAppMode{};
inline bool dark_mode_supported = false;
inline bool dark_mode_enabled = false;
inline DWORD build_number = 0;
inline HBRUSH bg_brush = nullptr;

template <typename T, typename T1, typename T2> inline constexpr T rva_to_va(T1 base, T2 rva)
{
    return reinterpret_cast<T>(reinterpret_cast<ULONG_PTR>(base) + rva);
}

template <typename T> inline constexpr T data_directory_from_module_base(void *moduleBase, size_t entryID)
{
    auto dosHdr = reinterpret_cast<PIMAGE_DOS_HEADER>(moduleBase);
    auto ntHdr = rva_to_va<PIMAGE_NT_HEADERS>(moduleBase, dosHdr->e_lfanew);
    auto dataDir = ntHdr->OptionalHeader.DataDirectory;
    return rva_to_va<T>(moduleBase, dataDir[entryID].VirtualAddress);
}

inline PIMAGE_THUNK_DATA find_address_by_name(void *moduleBase, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr,
                                              const char *funcName)
{
    for (; impName->u1.Ordinal; ++impName, ++impAddr)
    {
        if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal)) continue;

        auto import = rva_to_va<PIMAGE_IMPORT_BY_NAME>(moduleBase, impName->u1.AddressOfData);
        if (strcmp(import->Name, funcName) != 0) continue;
        return impAddr;
    }
    return nullptr;
}

inline PIMAGE_THUNK_DATA find_address_by_ordinal(void *moduleBase, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr,
                                                 uint16_t ordinal)
{
    for (; impName->u1.Ordinal; ++impName, ++impAddr)
    {
        if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal) && IMAGE_ORDINAL(impName->u1.Ordinal) == ordinal) return impAddr;
    }
    return nullptr;
}

inline PIMAGE_THUNK_DATA find_iat_thunk_in_module(void *moduleBase, const char *dllName, const char *funcName)
{
    auto imports = data_directory_from_module_base<PIMAGE_IMPORT_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_IMPORT);
    for (; imports->Name; ++imports)
    {
        if (_stricmp(rva_to_va<LPCSTR>(moduleBase, imports->Name), dllName) != 0) continue;

        auto origThunk = rva_to_va<PIMAGE_THUNK_DATA>(moduleBase, imports->OriginalFirstThunk);
        auto thunk = rva_to_va<PIMAGE_THUNK_DATA>(moduleBase, imports->FirstThunk);
        return find_address_by_name(moduleBase, origThunk, thunk, funcName);
    }
    return nullptr;
}

inline PIMAGE_THUNK_DATA find_delay_load_thunk_in_module(void *moduleBase, const char *dllName, const char *funcName)
{
    auto imports =
        data_directory_from_module_base<PIMAGE_DELAYLOAD_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
    for (; imports->DllNameRVA; ++imports)
    {
        if (_stricmp(rva_to_va<LPCSTR>(moduleBase, imports->DllNameRVA), dllName) != 0) continue;

        auto impName = rva_to_va<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportNameTableRVA);
        auto impAddr = rva_to_va<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportAddressTableRVA);
        return find_address_by_name(moduleBase, impName, impAddr, funcName);
    }
    return nullptr;
}

inline PIMAGE_THUNK_DATA find_delay_load_thunk_in_module(void *moduleBase, const char *dllName, uint16_t ordinal)
{
    auto imports =
        data_directory_from_module_base<PIMAGE_DELAYLOAD_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
    for (; imports->DllNameRVA; ++imports)
    {
        if (_stricmp(rva_to_va<LPCSTR>(moduleBase, imports->DllNameRVA), dllName) != 0) continue;

        auto impName = rva_to_va<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportNameTableRVA);
        auto impAddr = rva_to_va<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportAddressTableRVA);
        return find_address_by_ordinal(moduleBase, impName, impAddr, ordinal);
    }
    return nullptr;
}

inline bool is_high_contrast()
{
    HIGHCONTRAST high_contrast{};
    high_contrast.cbSize = sizeof(high_contrast);
    if (!SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(high_contrast), &high_contrast, FALSE)) return false;
    return high_contrast.dwFlags & HCF_HIGHCONTRASTON;
}

inline void refresh_titlebar(HWND hwnd)
{
    BOOL dark = FALSE;
    if (_IsDarkModeAllowedForWindow(hwnd) && _ShouldAppsUseDarkMode() && !is_high_contrast())
    {
        dark = TRUE;
    }

    if (build_number < 18362)
        SetProp(hwnd, L"UseImmersiveDarkModeColors", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(dark)));
    else if (_SetWindowCompositionAttribute)
    {
        WINDOWCOMPOSITIONATTRIBDATA data = {WCA_USEDARKMODECOLORS, &dark, sizeof(dark)};
        _SetWindowCompositionAttribute(hwnd, &data);
    }
}

inline bool is_theme_change_message(UINT message, LPARAM lparam)
{
    if (message != WM_SETTINGCHANGE) return false;

    bool is = false;
    const auto lparam_str = reinterpret_cast<LPCWCH>(lparam);
    if (lparam && CompareStringOrdinal(lparam_str, -1, L"ImmersiveColorSet", -1, TRUE) == CSTR_EQUAL)
    {
        _RefreshImmersiveColorPolicyState();
        is = true;
    }
    _GetIsImmersiveColorUsingHighContrast(IHCM_REFRESH);
    return is;
}

inline void force_explorer_scrollbar()
{
    HMODULE comctl_mod = LoadLibraryExW(L"comctl32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!comctl_mod) return;

    const auto addr = find_delay_load_thunk_in_module(comctl_mod, "uxtheme.dll", 49); // OpenNcThemeData
    if (!addr) return;

    DWORD prev_protect;
    if (!VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &prev_protect)) return;

    auto open_nc_theme_data_thunk = [](HWND hwnd, LPCWSTR class_list) -> HTHEME {
        if (wcscmp(class_list, L"ScrollBar") == 0)
        {
            hwnd = nullptr;
            class_list = L"Explorer::ScrollBar";
        }
        return _OpenNcThemeData(hwnd, class_list);
    };

    addr->u1.Function = reinterpret_cast<ULONG_PTR>(static_cast<fnOpenNcThemeData>(open_nc_theme_data_thunk));

    VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), prev_protect, &prev_protect);
}

inline void InitListView(HWND lv_hwnd)
{
    _AllowDarkModeForWindow(lv_hwnd, true);

    HWND hdr_hwnd = ListView_GetHeader(lv_hwnd);
    _AllowDarkModeForWindow(hdr_hwnd, true);

    const auto subclass = [](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/,
                             DWORD_PTR dwRefData) -> LRESULT {
        switch (uMsg)
        {
        case WM_NOTIFY: {
            if (reinterpret_cast<LPNMHDR>(lParam)->code == NM_CUSTOMDRAW)
            {
                LPNMCUSTOMDRAW nmcd = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);
                switch (nmcd->dwDrawStage)
                {
                case CDDS_PREPAINT:
                    return CDRF_NOTIFYITEMDRAW;
                case CDDS_ITEMPREPAINT: {
                    auto info = reinterpret_cast<SubclassInfo *>(dwRefData);
                    SetTextColor(nmcd->hdc, info->headerTextColor);
                    return CDRF_DODEFAULT;
                }
                }
            }
        }
        break;
        case WM_THEMECHANGED: {
            HWND hdr_hwnd = ListView_GetHeader(hWnd);
            HTHEME hTheme = OpenThemeData(nullptr, L"ItemsView");
            if (hTheme)
            {
                COLORREF color;
                if (SUCCEEDED(GetThemeColor(hTheme, 0, 0, TMT_TEXTCOLOR, &color)))
                {
                    ListView_SetTextColor(hWnd, color);
                }
                if (SUCCEEDED(GetThemeColor(hTheme, 0, 0, TMT_FILLCOLOR, &color)))
                {
                    ListView_SetTextBkColor(hWnd, color);
                    ListView_SetBkColor(hWnd, color);
                }
                CloseThemeData(hTheme);
            }

            hTheme = OpenThemeData(hdr_hwnd, L"Header");
            if (hTheme)
            {
                auto info = reinterpret_cast<SubclassInfo *>(dwRefData);
                GetThemeColor(hTheme, HP_HEADERITEM, 0, TMT_TEXTCOLOR, &(info->headerTextColor));
                CloseThemeData(hTheme);
            }

            SendMessage(hdr_hwnd, WM_THEMECHANGED, wParam, lParam);

            RedrawWindow(hWnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
        case WM_DESTROY: {
            auto info = reinterpret_cast<SubclassInfo *>(dwRefData);
            delete info;
        }
        break;
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    };

    SetWindowSubclass(lv_hwnd, subclass, 0, reinterpret_cast<DWORD_PTR>(new SubclassInfo{}));

    // FIXME: We force grid lines off because they look absolutely brutal in dark mode. Maybe we can override them?
    const auto ex_style = ListView_GetExtendedListViewStyle(lv_hwnd);
    ListView_SetExtendedListViewStyle(lv_hwnd, ex_style & ~LVS_EX_GRIDLINES);

    // FIXME: Hide focus rectangle because it's white :/ Would be nice to override it instead.
    SendMessage(lv_hwnd, WM_CHANGEUISTATE, MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);

    SetWindowTheme(hdr_hwnd, L"ItemsView", L"Header");
    SetWindowTheme(lv_hwnd, L"ItemsView", 0);
}

inline void apply_to_control(HWND hwnd)
{
    wchar_t cls[32]{};
    GetClassName(hwnd, cls, std::size(cls));
    std::wstring class_name(cls);

    // Don't touch the header, it's handled in InitListView.
    // FIXME: Can standalone header controls exist? If so, this will break them.
    if (class_name == WC_HEADER) return;

    if (class_name == WC_LISTVIEW)
    {
        InitListView(hwnd);
        return;
    }

    _AllowDarkModeForWindow(hwnd, true);
    SetWindowTheme(hwnd, L"Explorer", nullptr);
}

inline void apply_to_child_windows(HWND hwnd)
{
    EnumChildWindows(
        hwnd,
        [](HWND hwnd, LPARAM) -> BOOL {
            apply_to_control(hwnd);
            return TRUE;
        },
        0);
}

inline LRESULT CALLBACK wnd_subclass_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId,
                                          DWORD_PTR dwRefData)
{
    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, wnd_subclass_proc, sId);
        break;
    case WM_PARENTNOTIFY:
        switch (LOWORD(wParam))
        {
        case WM_CREATE: {
            const auto child_hwnd = reinterpret_cast<HWND>(lParam);
            apply_to_control(child_hwnd);
            break;
        }

        default:
            break;
        }
        break;
    default:
        break;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

inline LRESULT CALLBACK dlg_subclass_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId,
                                          DWORD_PTR dwRefData)
{
    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, dlg_subclass_proc, sId);
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC: {
        if (!_ShouldAppsUseDarkMode() || is_high_contrast()) break;
        if (!bg_brush) bg_brush = CreateSolidBrush(bg_color);

        const auto hdc = reinterpret_cast<HDC>(wParam);

        SetTextColor(hdc, text_color);
        SetBkColor(hdc, bg_color);

        return reinterpret_cast<INT_PTR>(bg_brush);
    }
    break;
    default:
        break;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

inline bool is_top_level_window(HWND hwnd)
{
    if (!IsWindow(hwnd)) return false;

    const auto style = GetWindowLongPtr(hwnd, GWL_STYLE);

    if ((style & WS_OVERLAPPEDWINDOW) || (style & WS_POPUP))
    {
        HWND parent = GetParent(hwnd);
        if (!parent) return true;
    }
    return false;
}

}; // namespace Internal

/**
 * @brief Options for `attach`. See `attach` for details.
 */
struct AttachOptions
{
    /**
     * @brief Whether the window is a dialog. This is used to apply some dialog-specific dark mode fixes. If
     * unspecified, the function will attempt to determine it automatically.
     */
    std::optional<bool> is_dialog = std::nullopt;
};

/**
 * @brief Initializes dark mode support. Call this once at the start of your program, preferrably in `WinMain` before
 * creating any windows.
 */
inline void init()
{
    using namespace Internal;

    auto RtlGetNtVersionNumbers = reinterpret_cast<fnRtlGetNtVersionNumbers>(
        GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlGetNtVersionNumbers"));
    if (!RtlGetNtVersionNumbers) return;

    DWORD major, minor;
    RtlGetNtVersionNumbers(&major, &minor, &build_number);
    build_number &= ~0xF0000000;

    HMODULE h_ut = LoadLibraryEx(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!h_ut) return;

    _OpenNcThemeData = reinterpret_cast<fnOpenNcThemeData>(GetProcAddress(h_ut, MAKEINTRESOURCEA(49)));
    _RefreshImmersiveColorPolicyState =
        reinterpret_cast<fnRefreshImmersiveColorPolicyState>(GetProcAddress(h_ut, MAKEINTRESOURCEA(104)));
    _GetIsImmersiveColorUsingHighContrast =
        reinterpret_cast<fnGetIsImmersiveColorUsingHighContrast>(GetProcAddress(h_ut, MAKEINTRESOURCEA(106)));
    _ShouldAppsUseDarkMode = reinterpret_cast<fnShouldAppsUseDarkMode>(GetProcAddress(h_ut, MAKEINTRESOURCEA(132)));
    _AllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(GetProcAddress(h_ut, MAKEINTRESOURCEA(133)));

    auto ord135 = GetProcAddress(h_ut, MAKEINTRESOURCEA(135));
    if (build_number < 18362)
        _AllowDarkModeForApp = reinterpret_cast<fnAllowDarkModeForApp>(ord135);
    else
        _SetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(ord135);

    _IsDarkModeAllowedForWindow =
        reinterpret_cast<fnIsDarkModeAllowedForWindow>(GetProcAddress(h_ut, MAKEINTRESOURCEA(137)));

    _SetWindowCompositionAttribute = reinterpret_cast<fnSetWindowCompositionAttribute>(
        GetProcAddress(GetModuleHandle(L"user32.dll"), "SetWindowCompositionAttribute"));

    if (_OpenNcThemeData && _RefreshImmersiveColorPolicyState && _ShouldAppsUseDarkMode && _AllowDarkModeForWindow &&
        (_AllowDarkModeForApp || _SetPreferredAppMode) && _IsDarkModeAllowedForWindow)
    {
        dark_mode_supported = true;

        if (_AllowDarkModeForApp)
            _AllowDarkModeForApp(true);
        else if (_SetPreferredAppMode)
            _SetPreferredAppMode(AllowDark);

        _RefreshImmersiveColorPolicyState();

        dark_mode_enabled = _ShouldAppsUseDarkMode() && !is_high_contrast();

        force_explorer_scrollbar();
    }
}

/**
 * @brief Attaches dark mode support to a window.
 * @param hwnd The handle to the window.
 * @param options Options for attaching. See `AttachOptions` for details.
 * @remarks Adding, changing, or removing child windows after this call is not properly supported yet.
 */
inline void attach(HWND hwnd, const AttachOptions &options = {})
{
    using namespace Internal;

    if (!dark_mode_supported) return;

    _AllowDarkModeForWindow(hwnd, true);
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark_mode_enabled, sizeof(dark_mode_enabled));

    refresh_titlebar(hwnd);

    apply_to_child_windows(hwnd);

    SetWindowSubclass(hwnd, wnd_subclass_proc, 0, 0);

    const bool is_dialog = options.is_dialog.value_or(!is_top_level_window(hwnd));
    if (is_dialog) SetWindowSubclass(hwnd, dlg_subclass_proc, 0, 0);
}

} // namespace WinDarkMode
