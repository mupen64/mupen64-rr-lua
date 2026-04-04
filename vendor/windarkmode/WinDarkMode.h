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
#include <unordered_set>

#ifndef _UNICODE
#error WinDarkMode requires a Unicode build.
#endif

/**
 * @brief Namespace containing functions for enabling and using dark mode on Win32.
 */
namespace WinDarkMode
{

/**
 * @brief Custom data associated with a theme.
 */
struct ThemeData
{
#define PAIR(name)                                                                                                     \
    COLORREF name##_color{};                                                                                           \
    HBRUSH name##_brush{};
    PAIR(bg);
    PAIR(text_1);
    PAIR(text_2);
    PAIR(listbox_bg);
    PAIR(edit_bg);
    PAIR(tab_normal);
    PAIR(tab_hover);
};

constexpr ThemeData light_theme_data = {.bg_color = RGB(255, 255, 255),
                                        .text_1_color = RGB(0, 0, 0),
                                        .text_2_color = RGB(255, 255, 255),
                                        .listbox_bg_color = RGB(255, 255, 255),
                                        .edit_bg_color = RGB(255, 255, 255),
                                        .tab_normal_color = RGB(243, 243, 243),
                                        .tab_hover_color = RGB(249, 249, 249)};

constexpr ThemeData dark_theme_data = {.bg_color = RGB(56, 56, 56),
                                       .text_1_color = RGB(255, 255, 255),
                                       .text_2_color = RGB(0, 0, 0),
                                       .listbox_bg_color = RGB(30, 30, 30),
                                       .edit_bg_color = RGB(30, 30, 30),
                                       .tab_normal_color = RGB(243, 243, 243),
                                       .tab_hover_color = RGB(249, 249, 249)};

inline ThemeData theme_data = light_theme_data;

/**
 * @brief The available themes.
 */
enum class Theme
{
    /**
     * @brief The default light theme.
     */
    Light,

    /**
     * @brief The dark theme.
     */
    Dark,

    /**
     * @brief The user's preferred theme. Updates dynamically when the user changes their system theme.
     */
    System
};

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

struct ListViewContext
{
    COLORREF hdr_text_color;
};

struct TabControlContext
{
    // TODO: Implement hover highlights
};

struct StatusBarContext
{
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

inline ULONG_PTR original_open_nc_theme_data{};

inline Theme theme = Theme::Light;
inline std::unordered_set<HWND> attached_windows;
inline bool dark_mode_supported = false;
inline DWORD build_number = 0;
inline HBRUSH bg_brush = nullptr;
inline HBRUSH listbox_bg_brush = nullptr;
inline HBRUSH listbox_fg_brush = nullptr;

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

inline Theme effective_theme()
{
    if (theme != Theme::System) return theme;

    if (_ShouldAppsUseDarkMode() && !is_high_contrast())
        return Theme::Dark;
    else
        return Theme::Light;
};

inline bool is_dark()
{
    return effective_theme() == Theme::Dark;
}

inline void refresh_titlebar(HWND hwnd, bool dark)
{
    if (build_number < 18362)
        SetProp(hwnd, L"UseImmersiveDarkModeColors", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(dark)));
    if (_SetWindowCompositionAttribute)
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

inline void patch_scrollbar(bool dark)
{
    HMODULE comctl_mod = LoadLibraryExW(L"comctl32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!comctl_mod) return;

    const auto addr = find_delay_load_thunk_in_module(comctl_mod, "uxtheme.dll", 49); // OpenNcThemeData
    if (!addr) return;

    DWORD prev_protect;
    if (!VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &prev_protect)) return;

    if (!original_open_nc_theme_data) original_open_nc_theme_data = addr->u1.Function;

    if (dark)
    {
        auto open_nc_theme_data_thunk = [](HWND hwnd, LPCWSTR class_list) -> HTHEME {
            if (wcscmp(class_list, L"ScrollBar") == 0)
            {
                hwnd = nullptr;
                class_list = L"Explorer::ScrollBar";
            }
            return _OpenNcThemeData(hwnd, class_list);
        };

        addr->u1.Function = reinterpret_cast<ULONG_PTR>(static_cast<fnOpenNcThemeData>(open_nc_theme_data_thunk));
    }
    else
    {
        addr->u1.Function = original_open_nc_theme_data;
    }

    VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), prev_protect, &prev_protect);
}

inline LRESULT CALLBACK tabcontrol_subclass_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId,
                                                 DWORD_PTR dwRefData)
{
    auto ctx = reinterpret_cast<TabControlContext *>(dwRefData);

    switch (msg)
    {
    case WM_NCDESTROY: {
        delete ctx;
        RemoveWindowSubclass(hwnd, tabcontrol_subclass_proc, 0);
        break;
    }
    case WM_ERASEBKGND: {
        if (!(GetWindowLongPtr(hwnd, GWL_STYLE) & TCS_OWNERDRAWFIXED)) break;

        RECT rc{};
        GetClientRect(hwnd, &rc);
        FillRect(reinterpret_cast<HDC>(wParam), &rc, theme_data.bg_brush);
        return TRUE;
    }

    case WM_PAINT: {
        if (!(GetWindowLongPtr(hwnd, GWL_STYLE) & TCS_OWNERDRAWFIXED)) break;

        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);

        FillRect(hdc, &ps.rcPaint, theme_data.bg_brush);

        const int nTabs = TabCtrl_GetItemCount(hwnd);
        const int nSelTab = TabCtrl_GetCurSel(hwnd);
        const UINT id = static_cast<UINT>(GetDlgCtrlID(hwnd));

        for (int i = 0; i < nTabs; i++)
        {
            DRAWITEMSTRUCT dis{};
            dis.CtlType = ODT_TAB;
            dis.CtlID = id;
            dis.itemID = static_cast<UINT>(i);
            dis.itemAction = ODA_DRAWENTIRE;
            dis.itemState = (i == nSelTab) ? ODS_SELECTED : ODS_DEFAULT;
            dis.hwndItem = hwnd;
            dis.hDC = hdc;
            TabCtrl_GetItemRect(hwnd, i, &dis.rcItem);

            RECT rcIntersect{};
            if (!IntersectRect(&rcIntersect, &ps.rcPaint, &dis.rcItem)) continue;

            const bool selected = (i == nSelTab);
            const COLORREF tabBg = selected ? RGB(0x50, 0x50, 0x50) : theme_data.bg_color;
            HBRUSH tabBrush = CreateSolidBrush(tabBg);
            FillRect(hdc, &dis.rcItem, tabBrush);
            DeleteObject(tabBrush);

            wchar_t label[256]{};
            TCITEMW tci{};
            tci.mask = TCIF_TEXT;
            tci.pszText = label;
            tci.cchTextMax = static_cast<int>(std::size(label)) - 1;
            TabCtrl_GetItem(hwnd, i, &tci);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, theme_data.text_1_color);
            HFONT hFont = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
            HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));
            DrawTextW(hdc, label, -1, &dis.rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            SelectObject(hdc, hOldFont);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }
    default:
        break;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

inline LRESULT CALLBACK listview_subclass_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId,
                                               DWORD_PTR dwRefData)
{
    auto info = reinterpret_cast<ListViewContext *>(dwRefData);

    switch (msg)
    {
    case WM_NCDESTROY: {
        delete info;
        RemoveWindowSubclass(hwnd, listview_subclass_proc, 0);
        break;
    }
    case WM_NOTIFY: {
        if (reinterpret_cast<LPNMHDR>(lParam)->code == NM_CUSTOMDRAW)
        {
            LPNMCUSTOMDRAW nmcd = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);
            switch (nmcd->dwDrawStage)
            {
            case CDDS_PREPAINT:
                return CDRF_NOTIFYITEMDRAW;
            case CDDS_ITEMPREPAINT: {
                SetTextColor(nmcd->hdc, info->hdr_text_color);
                return CDRF_DODEFAULT;
            }
            }
        }
        break;
    }
    default:
        break;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

inline LRESULT CALLBACK groupbox_subclass_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId,
                                               DWORD_PTR dwRefData)
{
    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, groupbox_subclass_proc, sId);
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc{};
        GetClientRect(hwnd, &rc);

        HFONT hFont = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
        HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));

        TEXTMETRIC tm{};
        GetTextMetrics(hdc, &tm);
        const int text_y_offset = tm.tmHeight / 2;

        wchar_t label[256]{};
        GetWindowTextW(hwnd, label, static_cast<int>(std::size(label)));

        const int text_padding = 4;
        SIZE text_size{};
        GetTextExtentPoint32W(hdc, label, static_cast<int>(wcslen(label)), &text_size);

        if (!bg_brush) bg_brush = CreateSolidBrush(theme_data.bg_color);
        FillRect(hdc, &rc, bg_brush);

        RECT frame_rc = rc;
        frame_rc.top += text_y_offset;

        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
        HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
        Rectangle(hdc, frame_rc.left, frame_rc.top, frame_rc.right, frame_rc.bottom);
        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hPen);

        if (label[0])
        {
            const int text_x = rc.left + 9;
            RECT clear_rc = {text_x - text_padding, rc.top, text_x + text_size.cx + text_padding, rc.top + tm.tmHeight};
            FillRect(hdc, &clear_rc, bg_brush);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, theme_data.text_1_color);
            RECT text_rc = {text_x, rc.top, text_x + text_size.cx, rc.top + tm.tmHeight};
            DrawTextW(hdc, label, -1, &text_rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
        }

        SelectObject(hdc, hOldFont);
        EndPaint(hwnd, &ps);
        return 0;
    }

    default:
        break;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

inline LRESULT CALLBACK statusbar_subclass_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId,
                                                DWORD_PTR dwRefData)
{
    auto *ctx = reinterpret_cast<StatusBarContext *>(dwRefData);

    switch (msg)
    {
    case WM_NCDESTROY:
        delete ctx;
        RemoveWindowSubclass(hwnd, statusbar_subclass_proc, sId);
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc_client{};
        GetClientRect(hwnd, &rc_client);

        if (!bg_brush) bg_brush = CreateSolidBrush(theme_data.bg_color);
        FillRect(hdc, &rc_client, bg_brush);

        const int part_count = static_cast<int>(SendMessage(hwnd, SB_GETPARTS, 0, 0));

        std::vector<int> right_edges(static_cast<size_t>(part_count));
        SendMessage(hwnd, SB_GETPARTS, static_cast<WPARAM>(part_count), reinterpret_cast<LPARAM>(right_edges.data()));

        int borders[3]{};
        SendMessage(hwnd, SB_GETBORDERS, 0, reinterpret_cast<LPARAM>(borders));

        HPEN divider_pen = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
        HPEN old_pen = static_cast<HPEN>(SelectObject(hdc, divider_pen));

        HFONT h_font = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
        HFONT h_old_font = static_cast<HFONT>(SelectObject(hdc, h_font));

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, theme_data.text_1_color);

        const COLORREF sep_color = RGB(60, 60, 60);
        HPEN sep_pen = CreatePen(PS_SOLID, 1, sep_color);
        HPEN tmp_pen = static_cast<HPEN>(SelectObject(hdc, sep_pen));
        MoveToEx(hdc, rc_client.left, rc_client.top, nullptr);
        LineTo(hdc, rc_client.right, rc_client.top);
        SelectObject(hdc, tmp_pen);
        DeleteObject(sep_pen);

        for (int i = 0; i < part_count; ++i)
        {
            RECT rc_part{};
            SendMessage(hwnd, SB_GETRECT, static_cast<WPARAM>(i), reinterpret_cast<LPARAM>(&rc_part));

            RECT rc_intersect{};
            if (!IntersectRect(&rc_intersect, &ps.rcPaint, &rc_part)) continue;

            FillRect(hdc, &rc_part, bg_brush);

            const bool is_last_part = (i == part_count - 1);
            if (!is_last_part)
            {
                SelectObject(hdc, divider_pen);
                MoveToEx(hdc, rc_part.right - 1, rc_part.top + 2, nullptr);
                LineTo(hdc, rc_part.right - 1, rc_part.bottom - 2);
            }

            HICON h_icon = reinterpret_cast<HICON>(SendMessage(hwnd, SB_GETICON, static_cast<WPARAM>(i), 0));

            RECT rc_text = rc_part;
            rc_text.left += borders[0];

            if (h_icon)
            {
                const int icon_size = GetSystemMetrics(SM_CYSMICON);
                const int icon_y = rc_part.top + (rc_part.bottom - rc_part.top - icon_size) / 2;
                DrawIconEx(hdc, rc_text.left, icon_y, h_icon, icon_size, icon_size, 0, nullptr, DI_NORMAL);
                rc_text.left += icon_size + borders[2];
            }

            const LRESULT text_info = SendMessage(hwnd, SB_GETTEXTLENGTH, static_cast<WPARAM>(i), 0);
            const int text_len = LOWORD(text_info);

            if (text_len > 0)
            {
                std::wstring text(static_cast<size_t>(text_len) + 1, L'\0');
                SendMessage(hwnd, SB_GETTEXT, static_cast<WPARAM>(i), reinterpret_cast<LPARAM>(text.data()));
                text.resize(static_cast<size_t>(text_len));

                rc_text.right -= borders[0];
                DrawTextW(hdc, text.c_str(), static_cast<int>(text.size()), &rc_text,
                          DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
            }
        }

        const auto bar_style = GetWindowLongPtr(hwnd, GWL_STYLE);
        if (bar_style & SBARS_SIZEGRIP)
        {
            constexpr int DOT = 2;
            constexpr int GAP = 2;
            constexpr int ROWS = 3;
            const COLORREF dot_color = RGB(120, 120, 120);
            HBRUSH dot_brush = CreateSolidBrush(dot_color);

            const int base_x = rc_client.right - 2;
            const int base_y = rc_client.bottom - 2;

            for (int row = 0; row < ROWS; ++row)
            {
                for (int col = 0; col <= row; ++col)
                {
                    const int x = base_x - col * (DOT + GAP) - DOT;
                    const int y = base_y - row * (DOT + GAP) - DOT;
                    RECT dot_rc = {x, y, x + DOT, y + DOT};
                    FillRect(hdc, &dot_rc, dot_brush);
                }
            }

            DeleteObject(dot_brush);
        }

        SelectObject(hdc, h_old_font);
        SelectObject(hdc, old_pen);
        DeleteObject(divider_pen);
        EndPaint(hwnd, &ps);
        return 0;
    }

    default:
        break;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

inline void update_listview(HWND lv_hwnd, bool dark)
{
    HWND hdr_hwnd = ListView_GetHeader(lv_hwnd);
    _AllowDarkModeForWindow(hdr_hwnd, dark);

    auto ctx = new ListViewContext{};
    if (dark)
        SetWindowSubclass(lv_hwnd, listview_subclass_proc, 0, reinterpret_cast<DWORD_PTR>(ctx));
    else
        RemoveWindowSubclass(lv_hwnd, listview_subclass_proc, 0);

    if (dark)
    {
        // FIXME: We force grid lines off because they look absolutely brutal in dark mode. Maybe we can override them?
        const auto ex_style = ListView_GetExtendedListViewStyle(lv_hwnd);
        ListView_SetExtendedListViewStyle(lv_hwnd, ex_style & ~LVS_EX_GRIDLINES);

        // FIXME: Hide focus rectangle because it's white :/ Would be nice to override it instead.
        SendMessage(lv_hwnd, WM_CHANGEUISTATE, MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);

        SetWindowTheme(hdr_hwnd, L"ItemsView", L"Header");
        SetWindowTheme(lv_hwnd, L"ItemsView", 0);
    }
    else
    {
        SetWindowTheme(hdr_hwnd, nullptr, nullptr);
        SetWindowTheme(lv_hwnd, nullptr, nullptr);
    }

    HTHEME hTheme = OpenThemeData(nullptr, L"ItemsView");
    if (hTheme)
    {
        COLORREF color;
        if (SUCCEEDED(GetThemeColor(hTheme, 0, 0, TMT_TEXTCOLOR, &color))) ListView_SetTextColor(lv_hwnd, color);
        if (SUCCEEDED(GetThemeColor(hTheme, 0, 0, TMT_FILLCOLOR, &color)))
        {
            ListView_SetTextBkColor(lv_hwnd, color);
            ListView_SetBkColor(lv_hwnd, color);
        }
        CloseThemeData(hTheme);
    }

    hTheme = OpenThemeData(hdr_hwnd, L"Header");
    if (hTheme)
    {
        GetThemeColor(hTheme, HP_HEADERITEM, 0, TMT_TEXTCOLOR, &(ctx->hdr_text_color));
        CloseThemeData(hTheme);
    }
}

inline void update_control(HWND hwnd, bool dark)
{
    wchar_t cls[32]{};
    GetClassName(hwnd, cls, std::size(cls));
    std::wstring class_name(cls);

    _AllowDarkModeForWindow(hwnd, dark);

    // Don't touch the header, it's handled in InitListView.
    // FIXME: Can standalone header controls exist? If so, this will break them.
    if (class_name == WC_HEADER) return;

    if (class_name == WC_LISTVIEW)
    {
        update_listview(hwnd, dark);
        return;
    }

    if (class_name == WC_TABCONTROL)
    {
        SetWindowTheme(hwnd, dark ? L"DarkMode_DarkTheme" : nullptr, nullptr);

        // We have to owner-draw it :(
        const auto style = GetWindowLongPtr(hwnd, GWL_STYLE);
        if (dark)
            SetWindowLongPtr(hwnd, GWL_STYLE, style | TCS_OWNERDRAWFIXED);
        else
            SetWindowLongPtr(hwnd, GWL_STYLE, style & ~TCS_OWNERDRAWFIXED);

        if (dark)
            SetWindowSubclass(hwnd, tabcontrol_subclass_proc, 0, 0);
        else
            RemoveWindowSubclass(hwnd, tabcontrol_subclass_proc, 0);

        InvalidateRect(hwnd, nullptr, TRUE);
        return;
    }

    if (class_name == WC_BUTTON)
    {
        const auto style = GetWindowLongPtr(hwnd, GWL_STYLE) & 0xFL;
        if (style == BS_GROUPBOX)
        {
            if (dark)
                SetWindowSubclass(hwnd, groupbox_subclass_proc, 0, 0);
            else
                RemoveWindowSubclass(hwnd, groupbox_subclass_proc, 0);
            return;
        }
    }

    if (class_name == STATUSCLASSNAME)
    {
        if (dark)
        {
            SetWindowTheme(hwnd, L"", L"");
            SetWindowSubclass(hwnd, statusbar_subclass_proc, 0, reinterpret_cast<DWORD_PTR>(new StatusBarContext{}));
        }
        else
        {
            RemoveWindowSubclass(hwnd, statusbar_subclass_proc, 0);
            SetWindowTheme(hwnd, nullptr, nullptr);
        }
        return;
    }

    const std::unordered_map<std::wstring, std::wstring> theme_map = {
        {WC_EDIT, L"DarkMode_DarkTheme"},
        {WC_COMBOBOX, L"DarkMode_DarkTheme"},
        {WC_BUTTON, L"DarkMode_Explorer"},
    };

    if (dark)
    {
        if (theme_map.contains(class_name))
            SetWindowTheme(hwnd, theme_map.at(class_name).c_str(), nullptr);
        else
            SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);
    }
    else
    {
        SetWindowTheme(hwnd, nullptr, nullptr);
    }
}

inline void update_children(HWND hwnd, bool dark)
{
    EnumChildWindows(
        hwnd,
        [](HWND hwnd, LPARAM lparam) -> BOOL {
            const auto dark = static_cast<bool>(lparam);
            update_control(hwnd, dark);
            return TRUE;
        },
        static_cast<LPARAM>(dark));
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
            const auto dark = is_dark();
            update_control(child_hwnd, dark);
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
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        const auto dark = is_dark();
        if (!dark) break;

        const auto hdc = reinterpret_cast<HDC>(wParam);

        SetTextColor(hdc, theme_data.text_1_color);
        SetBkColor(hdc, theme_data.bg_color);

        return reinterpret_cast<INT_PTR>(theme_data.bg_brush);
    }
    case WM_CTLCOLOREDIT: {
        const auto dark = is_dark();
        if (!dark) break;

        const auto hdc = reinterpret_cast<HDC>(wParam);

        SetTextColor(hdc, theme_data.text_1_color);
        SetBkColor(hdc, theme_data.edit_bg_color);

        return reinterpret_cast<INT_PTR>(theme_data.edit_bg_brush);
    }
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

inline void update_window_theme(HWND hwnd, bool dark)
{
    _AllowDarkModeForWindow(hwnd, dark);
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    refresh_titlebar(hwnd, dark);
    update_children(hwnd, dark);

    const auto prev_brush = (HBRUSH)GetClassLongPtr(hwnd, GCLP_HBRBACKGROUND);
    if (prev_brush && prev_brush != theme_data.bg_brush) DeleteObject(prev_brush);
    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)theme_data.bg_brush);

    InvalidateRect(hwnd, nullptr, false);
}

inline void update_theme_data(bool dark)
{
#define RECREATE(x)                                                                                                    \
    if (theme_data.x##_brush)                                                                                          \
    {                                                                                                                  \
        DeleteObject(theme_data.x##_brush);                                                                            \
        theme_data.x##_brush = nullptr;                                                                                \
    }                                                                                                                  \
    theme_data.x##_brush = CreateSolidBrush(dark ? dark_theme_data.x##_color : light_theme_data.x##_color);            \
    theme_data.x##_color = dark ? dark_theme_data.x##_color : light_theme_data.x##_color;

    RECREATE(bg)
    RECREATE(text_1)
    RECREATE(text_2)
    RECREATE(listbox_bg)
    RECREATE(edit_bg)
}

} // namespace Internal

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
    }
}

/**
 * @brief Attaches dark mode support to a window. If the window has already been attached to, no work will be done.
 * @param hwnd The handle to the window.
 * @param options Options for attaching. See `AttachOptions` for details.
 * @remarks Adding, changing, or removing child windows after this call is not properly supported yet.
 */
inline void attach(HWND hwnd, const AttachOptions &options = {})
{
    using namespace Internal;

    if (!dark_mode_supported || attached_windows.contains(hwnd)) return;

    attached_windows.insert(hwnd);

    const auto dark = is_dark();
    update_window_theme(hwnd, dark);

    update_children(hwnd, dark);
    SetWindowSubclass(hwnd, wnd_subclass_proc, 0, 0);

    const bool is_dialog = options.is_dialog.value_or(!is_top_level_window(hwnd));
    if (is_dialog) SetWindowSubclass(hwnd, dlg_subclass_proc, 0, 0);
}

/**
 * @brief Sets the app theme.
 * @param theme The theme to set.
 */
inline void set(Theme theme)
{
    const auto prev_dark = Internal::is_dark();

    Internal::theme = theme;

    using namespace Internal;

    const auto dark = is_dark();

    if (dark != prev_dark) update_theme_data(dark);

    if (_AllowDarkModeForApp) _AllowDarkModeForApp(dark);
    if (_SetPreferredAppMode) _SetPreferredAppMode(dark ? ForceDark : ForceLight);
    _RefreshImmersiveColorPolicyState();
    patch_scrollbar(dark);

    for (const auto &hwnd : attached_windows)
    {
        update_window_theme(hwnd, dark);
    }
}

} // namespace WinDarkMode
