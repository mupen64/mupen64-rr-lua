/*
 * Copyright (c) 2026, abart27 (https://github.com/abart27).
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * WinDarkMode - next
 * https://github.com/abart27/WinDarkMode
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

#include <windows.h>
#include <algorithm>
#include <uxtheme.h>
#include <commdlg.h>
#include <vssym32.h>
#include <dwmapi.h>
#include <winerror.h>
#include <bit>
#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
#define COLOR(name) COLORREF name##_color{};
    PAIR(bg);
    PAIR(text_1);
    PAIR(text_2);
    PAIR(listbox_bg);
    PAIR(edit_bg);
    PAIR(tab_normal);
    PAIR(tab_hover);
    PAIR(disabled_text);
    COLOR(groupbox_border);
    COLOR(statusbar_border);
    COLOR(statusbar_divider);
    COLOR(statusbar_grip);
#undef COLOR
#undef PAIR
};

constexpr ThemeData light_theme_data = {.bg_color = RGB(255, 255, 255),
    .text_1_color = RGB(0, 0, 0),
    .text_2_color = RGB(255, 255, 255),
    .listbox_bg_color = RGB(255, 255, 255),
    .edit_bg_color = RGB(255, 255, 255),
    .tab_normal_color = RGB(243, 243, 243),
    .tab_hover_color = RGB(249, 249, 249),
    .disabled_text_color = RGB(160, 160, 160),
    .groupbox_border_color = RGB(180, 180, 180),
    .statusbar_border_color = RGB(200, 200, 200),
    .statusbar_divider_color = RGB(180, 180, 180),
    .statusbar_grip_color = RGB(180, 180, 180)};

constexpr ThemeData dark_theme_data = {.bg_color = RGB(56, 56, 56),
    .text_1_color = RGB(255, 255, 255),
    .text_2_color = RGB(0, 0, 0),
    .listbox_bg_color = RGB(30, 30, 30),
    .edit_bg_color = RGB(30, 30, 30),
    .tab_normal_color = RGB(80, 80, 80),
    .tab_hover_color = RGB(95, 95, 95),
    .disabled_text_color = RGB(128, 128, 128),
    .groupbox_border_color = RGB(100, 100, 100),
    .statusbar_border_color = RGB(60, 60, 60),
    .statusbar_divider_color = RGB(80, 80, 80),
    .statusbar_grip_color = RGB(120, 120, 120)};

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

static constexpr UINT WDM_UAHDRAWMENU = 0x0091;
static constexpr UINT WDM_UAHDRAWMENUITEM = 0x0092;
static constexpr UINT WDM_REPAINT_SEPARATOR = WM_APP + 0x57;

struct UAHMENU
{
    HMENU hmenu;
    HDC hdc;
    DWORD dwFlags;
};

struct UAHMENUITEMMETRICS
{
    union {
        struct
        {
            DWORD cx;
            DWORD cy;
        } rgsizeBar[2];
        struct
        {
            DWORD cx;
            DWORD cy;
        } rgsizePopup[4];
    };
};

struct UAHMENUPOPUPMETRICS
{
    DWORD rgcx[4];
    DWORD fUpdateMaxWidths : 2;
};

struct UAHMENUITEM
{
    int iPosition;
    UAHMENUITEMMETRICS umim;
    UAHMENUPOPUPMETRICS umpm;
};

struct UAHDRAWMENUITEM
{
    DRAWITEMSTRUCT dis;
    UAHMENU um;
    UAHMENUITEM umi;
};

struct ListViewContext
{
    COLORREF hdr_text_color;
};

struct TabControlContext
{
    int hover = -1;
};

struct StatusBarContext
{
};

struct ButtonContext
{
    bool hot = false;
};

using fnRtlGetNtVersionNumbers = void(WINAPI *)(LPDWORD major, LPDWORD minor, LPDWORD build);
using fnSetWindowCompositionAttribute = BOOL(WINAPI *)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA *);

template <typename T> inline T get_proc_address(HMODULE module, LPCSTR name)
{
    return std::bit_cast<T>(GetProcAddress(module, name));
}

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

inline fnFlushMenuThemes _FlushMenuThemes{};
inline fnSetWindowCompositionAttribute _SetWindowCompositionAttribute{};
inline fnShouldAppsUseDarkMode _ShouldAppsUseDarkMode{};
inline fnAllowDarkModeForWindow _AllowDarkModeForWindow{};
inline fnAllowDarkModeForApp _AllowDarkModeForApp{};
inline fnRefreshImmersiveColorPolicyState _RefreshImmersiveColorPolicyState{};
inline fnIsDarkModeAllowedForWindow _IsDarkModeAllowedForWindow{};
inline fnGetIsImmersiveColorUsingHighContrast _GetIsImmersiveColorUsingHighContrast{};
inline fnOpenNcThemeData _OpenNcThemeData{};
inline fnShouldSystemUseDarkMode _ShouldSystemUseDarkMode{};
inline fnSetPreferredAppMode _SetPreferredAppMode{};

inline HMODULE h_uxtheme{};
inline ULONG_PTR original_open_nc_theme_data{};

inline bool initialized = false;
inline Theme theme = Theme::System;
inline std::unordered_map<HWND, std::vector<HWND>> attached_windows;
inline std::unordered_set<HWND> pending_separator_repaint;
inline bool dark_mode_supported = false;
inline DWORD build_number = 0;

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

inline PIMAGE_THUNK_DATA find_address_by_name(
    void *moduleBase, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr, const char *funcName)
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

inline PIMAGE_THUNK_DATA find_address_by_ordinal(PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr, uint16_t ordinal)
{
    for (; impName->u1.Ordinal; ++impName, ++impAddr)
    {
        if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal) && IMAGE_ORDINAL(impName->u1.Ordinal) == ordinal) return impAddr;
    }
    return nullptr;
}

inline bool equal_ignore_case(const char *lhs, const char *rhs)
{
    while (*lhs && *rhs)
    {
        if (std::tolower(static_cast<unsigned char>(*lhs)) != std::tolower(static_cast<unsigned char>(*rhs)))
            return false;
        ++lhs;
        ++rhs;
    }
    return *lhs == *rhs;
}

inline PIMAGE_THUNK_DATA find_iat_thunk_in_module(void *moduleBase, const char *dllName, const char *funcName)
{
    auto imports = data_directory_from_module_base<PIMAGE_IMPORT_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_IMPORT);
    for (; imports->Name; ++imports)
    {
        if (!equal_ignore_case(rva_to_va<LPCSTR>(moduleBase, imports->Name), dllName)) continue;

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
        if (!equal_ignore_case(rva_to_va<LPCSTR>(moduleBase, imports->DllNameRVA), dllName)) continue;

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
        if (!equal_ignore_case(rva_to_va<LPCSTR>(moduleBase, imports->DllNameRVA), dllName)) continue;

        auto impName = rva_to_va<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportNameTableRVA);
        auto impAddr = rva_to_va<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportAddressTableRVA);
        return find_address_by_ordinal(impName, impAddr, ordinal);
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

inline void paint_menu_separator(HWND hwnd)
{
    if (!GetMenu(hwnd)) return;

    MENUBARINFO mbi{};
    mbi.cbSize = sizeof(mbi);
    if (!GetMenuBarInfo(hwnd, OBJID_MENU, 0, &mbi)) return;

    RECT rc_window{};
    GetWindowRect(hwnd, &rc_window);

    HDC hdc = GetWindowDC(hwnd);
    if (hdc)
    {
        RECT rc_sep = {mbi.rcBar.left - rc_window.left, mbi.rcBar.bottom - rc_window.top,
            mbi.rcBar.right - rc_window.left, mbi.rcBar.bottom - rc_window.top + 1};
        FillRect(hdc, &rc_sep, theme_data.bg_brush);
        ReleaseDC(hwnd, hdc);
    }
}

inline void refresh_titlebar(HWND hwnd, bool dark)
{
    if (build_number < 18362)
        SetProp(hwnd, TEXT("UseImmersiveDarkModeColors"), reinterpret_cast<HANDLE>(static_cast<INT_PTR>(dark)));
    if (_SetWindowCompositionAttribute)
    {
        BOOL dark2 = dark;
        WINDOWCOMPOSITIONATTRIBDATA data = {WCA_USEDARKMODECOLORS, &dark2, sizeof(dark2)};
        _SetWindowCompositionAttribute(hwnd, &data);
    }
}

inline bool is_theme_change_message(UINT message, LPARAM lparam)
{
    if (message != WM_SETTINGCHANGE) return false;

    bool is = false;
    const auto lparam_str = reinterpret_cast<LPCTSTR>(lparam);
    if (lparam && lstrcmpi(lparam_str, TEXT("ImmersiveColorSet")) == 0)
    {
        _RefreshImmersiveColorPolicyState();
        is = true;
    }
    _GetIsImmersiveColorUsingHighContrast(IHCM_REFRESH);
    return is;
}

inline void patch_scrollbar(bool dark)
{
    HMODULE comctl_mod = LoadLibraryEx(TEXT("comctl32.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!comctl_mod) return;

    const auto addr = find_delay_load_thunk_in_module(comctl_mod, "uxtheme.dll", 49); // OpenNcThemeData
    if (!addr)
    {
        FreeLibrary(comctl_mod);
        return;
    }

    DWORD prev_protect;
    if (!VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &prev_protect))
    {
        FreeLibrary(comctl_mod);
        return;
    }

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
    FreeLibrary(comctl_mod);
}

inline LRESULT CALLBACK tabcontrol_subclass_proc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR dwRefData)
{
    auto ctx = reinterpret_cast<TabControlContext *>(dwRefData);

    switch (msg)
    {
    case WM_NCDESTROY: {
        delete ctx;
        RemoveWindowSubclass(hwnd, tabcontrol_subclass_proc, 0);
        break;
    }
    case WM_MOUSEMOVE: {
        TCHITTESTINFO hit_info{};
        hit_info.pt.x = static_cast<short>(LOWORD(lParam));
        hit_info.pt.y = static_cast<short>(HIWORD(lParam));
        const int hover = TabCtrl_HitTest(hwnd, &hit_info);
        if (ctx->hover == hover) return 0;

        const int previous_hover = ctx->hover;
        ctx->hover = hover;
        TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hwnd, 0};
        TrackMouseEvent(&tme);

        RECT rc{};
        if (previous_hover >= 0 && TabCtrl_GetItemRect(hwnd, previous_hover, &rc)) InvalidateRect(hwnd, &rc, FALSE);
        if (hover >= 0 && TabCtrl_GetItemRect(hwnd, hover, &rc)) InvalidateRect(hwnd, &rc, FALSE);
        return 0;
    }
    case WM_MOUSELEAVE:
        if (ctx->hover >= 0)
        {
            RECT rc{};
            if (TabCtrl_GetItemRect(hwnd, ctx->hover, &rc)) InvalidateRect(hwnd, &rc, FALSE);
            ctx->hover = -1;
        }
        return 0;
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
            const bool hovered = (i == ctx->hover);
            HBRUSH tab_brush = selected ? theme_data.tab_normal_brush : theme_data.bg_brush;
            if (hovered && !selected) tab_brush = theme_data.tab_hover_brush;

            HPEN tab_pen = CreatePen(PS_SOLID, 1, theme_data.tab_hover_color);
            HPEN old_pen = static_cast<HPEN>(SelectObject(hdc, tab_pen));
            HBRUSH old_brush = static_cast<HBRUSH>(SelectObject(hdc, tab_brush));
            constexpr int corner_radius = 6;
            RoundRect(hdc, dis.rcItem.left, dis.rcItem.top, dis.rcItem.right, dis.rcItem.bottom, corner_radius,
                corner_radius);

            RECT bottom_rc = dis.rcItem;
            bottom_rc.top = bottom_rc.bottom - corner_radius;
            FillRect(hdc, &bottom_rc, tab_brush);
            MoveToEx(hdc, dis.rcItem.left, dis.rcItem.top + corner_radius / 2, nullptr);
            LineTo(hdc, dis.rcItem.left, dis.rcItem.bottom);
            MoveToEx(hdc, dis.rcItem.right - 1, dis.rcItem.top + corner_radius / 2, nullptr);
            LineTo(hdc, dis.rcItem.right - 1, dis.rcItem.bottom);
            MoveToEx(hdc, dis.rcItem.left, dis.rcItem.bottom - 1, nullptr);
            LineTo(hdc, dis.rcItem.right, dis.rcItem.bottom - 1);

            SelectObject(hdc, old_brush);
            SelectObject(hdc, old_pen);
            DeleteObject(tab_pen);

            TCHAR label[256]{};
            TCITEM tci{};
            tci.mask = TCIF_TEXT;
            tci.pszText = label;
            tci.cchTextMax = static_cast<int>(std::size(label)) - 1;
            TabCtrl_GetItem(hwnd, i, &tci);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, theme_data.text_1_color);
            HFONT hFont = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
            HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));
            DrawText(hdc, label, -1, &dis.rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
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

inline LRESULT CALLBACK listview_subclass_proc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR dwRefData)
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

inline LRESULT CALLBACK groupbox_subclass_proc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId, DWORD_PTR)
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

        TCHAR label[256]{};
        GetWindowText(hwnd, label, static_cast<int>(std::size(label)));

        const int text_padding = 4;
        SIZE text_size{};
        GetTextExtentPoint32(hdc, label, lstrlen(label), &text_size);

        FillRect(hdc, &rc, theme_data.bg_brush);

        RECT frame_rc = rc;
        frame_rc.top += text_y_offset;

        HPEN hPen = CreatePen(PS_SOLID, 1, theme_data.groupbox_border_color);
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
            FillRect(hdc, &clear_rc, theme_data.bg_brush);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, theme_data.text_1_color);
            RECT text_rc = {text_x, rc.top, text_x + text_size.cx, rc.top + tm.tmHeight};
            DrawText(hdc, label, -1, &text_rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
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

inline LRESULT CALLBACK button_subclass_proc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId, DWORD_PTR dwRefData)
{
    auto *ctx = reinterpret_cast<ButtonContext *>(dwRefData);
    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, button_subclass_proc, sId);
        delete ctx;
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_MOUSEMOVE:
        if (!ctx->hot)
        {
            ctx->hot = true;
            TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hwnd, 0};
            TrackMouseEvent(&tme);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        break;
    case WM_MOUSELEAVE:
        ctx->hot = false;
        InvalidateRect(hwnd, nullptr, FALSE);
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        const HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        FillRect(hdc, &rc, theme_data.bg_brush);

        const auto btn_style = GetWindowLongPtr(hwnd, GWL_STYLE) & 0xFL;
        const bool is_radio = (btn_style == BS_RADIOBUTTON || btn_style == BS_AUTORADIOBUTTON);
        const int part = is_radio ? BP_RADIOBUTTON : BP_CHECKBOX;

        const LRESULT check_state = SendMessage(hwnd, BM_GETCHECK, 0, 0);
        const bool enabled = IsWindowEnabled(hwnd) != 0;

        int state;
        if (is_radio)
        {
            if (!enabled)
                state = check_state == BST_CHECKED ? RBS_CHECKEDDISABLED : RBS_UNCHECKEDDISABLED;
            else if (ctx->hot)
                state = check_state == BST_CHECKED ? RBS_CHECKEDHOT : RBS_UNCHECKEDHOT;
            else
                state = check_state == BST_CHECKED ? RBS_CHECKEDNORMAL : RBS_UNCHECKEDNORMAL;
        }
        else
        {
            if (!enabled)
                state = check_state == BST_CHECKED         ? CBS_CHECKEDDISABLED
                        : check_state == BST_INDETERMINATE ? CBS_MIXEDDISABLED
                                                           : CBS_UNCHECKEDDISABLED;
            else if (ctx->hot)
                state = check_state == BST_CHECKED         ? CBS_CHECKEDHOT
                        : check_state == BST_INDETERMINATE ? CBS_MIXEDHOT
                                                           : CBS_UNCHECKEDHOT;
            else
                state = check_state == BST_CHECKED         ? CBS_CHECKEDNORMAL
                        : check_state == BST_INDETERMINATE ? CBS_MIXEDNORMAL
                                                           : CBS_UNCHECKEDNORMAL;
        }

        HTHEME hTheme = OpenThemeData(hwnd, L"BUTTON");
        SIZE glyph_size = {GetSystemMetrics(SM_CYMENUCHECK), GetSystemMetrics(SM_CYMENUCHECK)};

        if (hTheme)
        {
            SIZE sz = {};
            if (SUCCEEDED(GetThemePartSize(hTheme, hdc, part, state, nullptr, TS_DRAW, &sz))) glyph_size = sz;

            const int gy = rc.top + (rc.bottom - rc.top - glyph_size.cy) / 2;
            const RECT glyph_rc = {rc.left, gy, rc.left + glyph_size.cx, gy + glyph_size.cy};
            DrawThemeBackground(hTheme, hdc, part, state, &glyph_rc, nullptr);
            CloseThemeData(hTheme);
        }

        TCHAR label[256] = {};
        GetWindowText(hwnd, label, _countof(label));
        if (label[0])
        {
            const RECT text_rc = {rc.left + glyph_size.cx + 4, rc.top, rc.right, rc.bottom};
            const HFONT hFont = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
            const HFONT hOldFont = hFont ? reinterpret_cast<HFONT>(SelectObject(hdc, hFont)) : nullptr;
            SetTextColor(hdc, enabled ? theme_data.text_1_color : theme_data.disabled_text_color);
            SetBkMode(hdc, TRANSPARENT);
            DrawText(hdc, label, -1, const_cast<LPRECT>(&text_rc), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            if (hOldFont) SelectObject(hdc, hOldFont);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }
    default:
        break;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

inline LRESULT CALLBACK statusbar_subclass_proc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId, DWORD_PTR dwRefData)
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

        FillRect(hdc, &rc_client, theme_data.bg_brush);

        const int part_count = static_cast<int>(SendMessage(hwnd, SB_GETPARTS, 0, 0));

        std::vector<int> right_edges(static_cast<size_t>(part_count));
        SendMessage(hwnd, SB_GETPARTS, static_cast<WPARAM>(part_count), reinterpret_cast<LPARAM>(right_edges.data()));

        int borders[3]{};
        SendMessage(hwnd, SB_GETBORDERS, 0, reinterpret_cast<LPARAM>(borders));

        HPEN divider_pen = CreatePen(PS_SOLID, 1, theme_data.statusbar_divider_color);
        HPEN old_pen = static_cast<HPEN>(SelectObject(hdc, divider_pen));

        HFONT h_font = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
        HFONT h_old_font = static_cast<HFONT>(SelectObject(hdc, h_font));

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, theme_data.text_1_color);

        HPEN sep_pen = CreatePen(PS_SOLID, 1, theme_data.statusbar_border_color);
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

            FillRect(hdc, &rc_part, theme_data.bg_brush);

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
                std::basic_string<TCHAR> text(static_cast<size_t>(text_len) + 1, TEXT('\0'));
                SendMessage(hwnd, SB_GETTEXT, static_cast<WPARAM>(i), reinterpret_cast<LPARAM>(text.data()));
                text.resize(static_cast<size_t>(text_len));

                rc_text.right -= borders[0];
                DrawText(hdc, text.c_str(), static_cast<int>(text.size()), &rc_text,
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
            }
        }

        const auto bar_style = GetWindowLongPtr(hwnd, GWL_STYLE);
        if (bar_style & SBARS_SIZEGRIP)
        {
            constexpr int DOT = 2;
            constexpr int GAP = 2;
            constexpr int ROWS = 3;
            HBRUSH dot_brush = CreateSolidBrush(theme_data.statusbar_grip_color);

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

    DWORD_PTR old_data = 0;
    if (GetWindowSubclass(lv_hwnd, listview_subclass_proc, 0, &old_data))
        delete reinterpret_cast<ListViewContext *>(old_data);

    ListViewContext *ctx = nullptr;
    if (dark)
    {
        ctx = new ListViewContext{};
        SetWindowSubclass(lv_hwnd, listview_subclass_proc, 0, reinterpret_cast<DWORD_PTR>(ctx));

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
        RemoveWindowSubclass(lv_hwnd, listview_subclass_proc, 0);
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

    if (ctx)
    {
        hTheme = OpenThemeData(hdr_hwnd, L"Header");
        if (hTheme)
        {
            GetThemeColor(hTheme, HP_HEADERITEM, 0, TMT_TEXTCOLOR, &(ctx->hdr_text_color));
            CloseThemeData(hTheme);
        }
    }
}

inline void update_control(HWND hwnd, bool dark, const std::vector<HWND> &exclude)
{
    TCHAR cls[32]{};
    GetClassName(hwnd, cls, std::size(cls));
    std::basic_string<TCHAR> class_name(cls);

    if (std::find(exclude.begin(), exclude.end(), hwnd) != exclude.end()) return;

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

        if (dark && TabCtrl_GetItemCount(hwnd) > 0)
        {
            RECT item_rc{};
            if (TabCtrl_GetItemRect(hwnd, 0, &item_rc))
            {
                const int item_width = item_rc.right - item_rc.left;
                const int item_height = item_rc.bottom - item_rc.top;
                const int width = item_width < 60 ? 60 : item_width;
                const int height = item_height < 24 ? 24 : item_height;
                if (width != item_width || height != item_height) TabCtrl_SetItemSize(hwnd, width, height);
            }
        }

        DWORD_PTR old_data = 0;
        if (GetWindowSubclass(hwnd, tabcontrol_subclass_proc, 0, &old_data))
        {
            RemoveWindowSubclass(hwnd, tabcontrol_subclass_proc, 0);
            delete reinterpret_cast<TabControlContext *>(old_data);
        }

        if (dark)
        {
            auto *ctx = new TabControlContext{};
            if (!SetWindowSubclass(hwnd, tabcontrol_subclass_proc, 0, reinterpret_cast<DWORD_PTR>(ctx))) delete ctx;
        }

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

        const bool is_check_or_radio = style == BS_CHECKBOX || style == BS_AUTOCHECKBOX || style == BS_3STATE ||
                                       style == BS_AUTO3STATE || style == BS_RADIOBUTTON || style == BS_AUTORADIOBUTTON;
        if (is_check_or_radio)
        {
            DWORD_PTR old_data = 0;
            if (GetWindowSubclass(hwnd, button_subclass_proc, 0, &old_data))
            {
                RemoveWindowSubclass(hwnd, button_subclass_proc, 0);
                delete reinterpret_cast<ButtonContext *>(old_data);
            }

            SetWindowTheme(hwnd, dark ? L"DarkMode_DarkTheme" : nullptr, nullptr);
            if (dark)
                SetWindowSubclass(hwnd, button_subclass_proc, 0, reinterpret_cast<DWORD_PTR>(new ButtonContext{}));

            InvalidateRect(hwnd, nullptr, TRUE);
            return;
        }
    }

    if (class_name == STATUSCLASSNAME)
    {
        DWORD_PTR old_data = 0;
        if (GetWindowSubclass(hwnd, statusbar_subclass_proc, 0, &old_data))
            delete reinterpret_cast<StatusBarContext *>(old_data);

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

    static const std::unordered_map<std::basic_string<TCHAR>, std::wstring> theme_map = {};

    if (dark)
    {
        if (theme_map.contains(class_name))
            SetWindowTheme(hwnd, theme_map.at(class_name).c_str(), nullptr);
        else
            SetWindowTheme(hwnd, L"DarkMode_DarkTheme", nullptr);
    }
    else
    {
        SetWindowTheme(hwnd, nullptr, nullptr);
    }
}

struct UpdateChildrenContext
{
    bool dark;
    const std::vector<HWND> *exclude;
};

inline void update_children(HWND hwnd, bool dark, const std::vector<HWND> &exclude)
{
    UpdateChildrenContext ctx{dark, &exclude};
    EnumChildWindows(
        hwnd,
        [](HWND hwnd, LPARAM lparam) -> BOOL {
            const auto *ctx = reinterpret_cast<const UpdateChildrenContext *>(lparam);
            update_control(hwnd, ctx->dark, *ctx->exclude);
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&ctx));
}

inline void update_theme_data(bool dark);
inline void update_window_theme(HWND hwnd, bool dark, const std::vector<HWND> &exclude);

inline LRESULT CALLBACK wnd_subclass_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId, DWORD_PTR)
{
    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, wnd_subclass_proc, sId);
        attached_windows.erase(hwnd);
        pending_separator_repaint.erase(hwnd);
        break;
    case WM_ERASEBKGND: {
        RECT client_rect{};
        GetClientRect(hwnd, &client_rect);
        const auto brush = theme_data.bg_brush ? theme_data.bg_brush : GetSysColorBrush(COLOR_WINDOW);
        FillRect(reinterpret_cast<HDC>(wParam), &client_rect, brush);
        return TRUE;
    }
    case WM_SETTINGCHANGE:
        if (theme == Theme::System && is_theme_change_message(msg, lParam))
        {
            const auto dark = is_dark();
            update_theme_data(dark);
            if (_FlushMenuThemes) _FlushMenuThemes();
            patch_scrollbar(dark);
            const auto it = attached_windows.find(hwnd);
            const std::vector<HWND> empty_exclude;
            const auto &exclude = it != attached_windows.end() ? it->second : empty_exclude;
            update_window_theme(hwnd, dark, exclude);
        }
        break;
    case WM_NCPAINT: {
        const LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        if (!is_dark()) return result;

        paint_menu_separator(hwnd);

        if (!pending_separator_repaint.contains(hwnd))
        {
            pending_separator_repaint.insert(hwnd);
            PostMessage(hwnd, WDM_REPAINT_SEPARATOR, 0, 0);
        }
        return result;
    }
    case WM_NCACTIVATE: {
        const LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        if (is_dark()) paint_menu_separator(hwnd);
        return result;
    }
    case WM_PARENTNOTIFY:
        switch (LOWORD(wParam))
        {
        case WM_CREATE: {
            const auto child_hwnd = reinterpret_cast<HWND>(lParam);
            const auto dark = is_dark();
            const auto it = attached_windows.find(hwnd);
            const std::vector<HWND> empty_exclude;
            const auto &exclude = it != attached_windows.end() ? it->second : empty_exclude;
            update_control(child_hwnd, dark, exclude);
            break;
        }
        default:
            break;
        }
        break;
    default:
        if (msg == WDM_REPAINT_SEPARATOR)
        {
            pending_separator_repaint.erase(hwnd);
            if (is_dark()) paint_menu_separator(hwnd);
            return 0;
        }
        if (msg == WDM_UAHDRAWMENU)
        {
            if (!is_dark()) break;
            auto *udm = reinterpret_cast<UAHMENU *>(lParam);
            MENUBARINFO mbi{};
            mbi.cbSize = sizeof(mbi);
            if (!GetMenuBarInfo(hwnd, OBJID_MENU, 0, &mbi)) break;
            RECT rc_win{};
            GetWindowRect(hwnd, &rc_win);
            RECT rc_menu = mbi.rcBar;
            OffsetRect(&rc_menu, -rc_win.left, -rc_win.top);
            rc_menu.bottom += 1;
            FillRect(udm->hdc, &rc_menu, theme_data.bg_brush);
            return TRUE;
        }
        if (msg == WDM_UAHDRAWMENUITEM)
        {
            if (!is_dark()) break;
            auto *udmi = reinterpret_cast<UAHDRAWMENUITEM *>(lParam);

            const bool hot = (udmi->dis.itemState & ODS_HOTLIGHT) != 0;
            const bool selected = (udmi->dis.itemState & ODS_SELECTED) != 0;

            FillRect(
                udmi->um.hdc, &udmi->dis.rcItem, (hot || selected) ? theme_data.tab_normal_brush : theme_data.bg_brush);

            TCHAR text[256]{};
            MENUITEMINFO mii{};
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_STRING;
            mii.dwTypeData = text;
            mii.cch = static_cast<UINT>(std::size(text));
            GetMenuItemInfo(udmi->um.hmenu, static_cast<UINT>(udmi->umi.iPosition), TRUE, &mii);

            NONCLIENTMETRICS ncm{};
            ncm.cbSize = sizeof(ncm);
            SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
            HFONT hFont = CreateFontIndirect(&ncm.lfMenuFont);
            HFONT hOldFont = static_cast<HFONT>(SelectObject(udmi->um.hdc, hFont));

            SetBkMode(udmi->um.hdc, TRANSPARENT);
            SetTextColor(udmi->um.hdc, theme_data.text_1_color);
            const UINT dt_flags =
                DT_CENTER | DT_VCENTER | DT_SINGLELINE | ((udmi->dis.itemState & ODS_NOACCEL) ? DT_HIDEPREFIX : 0U);
            DrawText(udmi->um.hdc, text, -1, &udmi->dis.rcItem, dt_flags);

            SelectObject(udmi->um.hdc, hOldFont);
            DeleteObject(hFont);
            return TRUE;
        }
        break;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

inline LRESULT CALLBACK dlg_subclass_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId, DWORD_PTR)
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

        const auto it = attached_windows.find(hwnd);
        const std::vector<HWND> empty_exclude;
        const auto &exclude = it != attached_windows.end() ? it->second : empty_exclude;
        if (msg != WM_CTLCOLORDLG)
        {
            const auto child_hwnd = reinterpret_cast<HWND>(lParam);
            if (child_hwnd && std::find(exclude.begin(), exclude.end(), child_hwnd) != exclude.end()) break;
        }

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

inline void update_window_theme(HWND hwnd, bool dark, const std::vector<HWND> &exclude)
{
    _AllowDarkModeForWindow(hwnd, dark);
    BOOL dark2 = dark;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark2, sizeof(dark2));
    refresh_titlebar(hwnd, dark);
    update_children(hwnd, dark, exclude);
    DrawMenuBar(hwnd);

    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)theme_data.bg_brush);

    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
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
    RECREATE(tab_normal)
    RECREATE(tab_hover)
    RECREATE(disabled_text)

#define UPDATE_COLOR(x) theme_data.x##_color = dark ? dark_theme_data.x##_color : light_theme_data.x##_color;
    UPDATE_COLOR(groupbox_border)
    UPDATE_COLOR(statusbar_border)
    UPDATE_COLOR(statusbar_divider)
    UPDATE_COLOR(statusbar_grip)
#undef UPDATE_COLOR
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

    /**
     * @brief Controls to exclude from dark mode.
     * The host application becomes responsible for properly coloring these controls, for which it can use `theme_data`.
     */
    std::vector<HWND> exclude;
};

inline void set(Theme theme);

/**
 * @brief Initializes dark mode support. Call this once at the start of your program, preferrably in `WinMain` before
 * creating any windows.
 */
inline void init()
{
    using namespace Internal;

    if (initialized) return;

    initialized = true;

    auto RtlGetNtVersionNumbers =
        get_proc_address<fnRtlGetNtVersionNumbers>(GetModuleHandle(TEXT("ntdll.dll")), "RtlGetNtVersionNumbers");
    if (!RtlGetNtVersionNumbers) return;

    DWORD major, minor;
    RtlGetNtVersionNumbers(&major, &minor, &build_number);
    build_number &= ~0xF0000000;

    h_uxtheme = LoadLibraryEx(TEXT("uxtheme.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!h_uxtheme) return;

    _OpenNcThemeData = get_proc_address<fnOpenNcThemeData>(h_uxtheme, MAKEINTRESOURCEA(49));
    _RefreshImmersiveColorPolicyState =
        get_proc_address<fnRefreshImmersiveColorPolicyState>(h_uxtheme, MAKEINTRESOURCEA(104));
    _GetIsImmersiveColorUsingHighContrast =
        get_proc_address<fnGetIsImmersiveColorUsingHighContrast>(h_uxtheme, MAKEINTRESOURCEA(106));
    _ShouldAppsUseDarkMode = get_proc_address<fnShouldAppsUseDarkMode>(h_uxtheme, MAKEINTRESOURCEA(132));
    _AllowDarkModeForWindow = get_proc_address<fnAllowDarkModeForWindow>(h_uxtheme, MAKEINTRESOURCEA(133));

    _FlushMenuThemes = get_proc_address<fnFlushMenuThemes>(h_uxtheme, MAKEINTRESOURCEA(136));

    if (build_number < 18362)
        _AllowDarkModeForApp = get_proc_address<fnAllowDarkModeForApp>(h_uxtheme, MAKEINTRESOURCEA(135));
    else
        _SetPreferredAppMode = get_proc_address<fnSetPreferredAppMode>(h_uxtheme, MAKEINTRESOURCEA(135));

    _IsDarkModeAllowedForWindow = get_proc_address<fnIsDarkModeAllowedForWindow>(h_uxtheme, MAKEINTRESOURCEA(137));

    _SetWindowCompositionAttribute = get_proc_address<fnSetWindowCompositionAttribute>(
        GetModuleHandle(TEXT("user32.dll")), "SetWindowCompositionAttribute");

    if (_OpenNcThemeData && _RefreshImmersiveColorPolicyState && _ShouldAppsUseDarkMode && _AllowDarkModeForWindow &&
        (_AllowDarkModeForApp || _SetPreferredAppMode) && _IsDarkModeAllowedForWindow)
    {
        dark_mode_supported = true;
        update_theme_data(is_dark());
    }

    WinDarkMode::set(theme);
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

    attached_windows.emplace(hwnd, options.exclude);

    const auto dark = is_dark();
    update_theme_data(dark);
    if (_FlushMenuThemes) _FlushMenuThemes();
    patch_scrollbar(dark);
    SetWindowSubclass(hwnd, wnd_subclass_proc, 0, 0);

    const bool is_dialog = options.is_dialog.value_or(!is_top_level_window(hwnd));
    if (is_dialog) SetWindowSubclass(hwnd, dlg_subclass_proc, 0, 0);

    update_window_theme(hwnd, dark, options.exclude);
}

/**
 * @brief Sets the app theme.
 * @param theme The theme to set.
 */
inline void set(Theme theme)
{
    if (!Internal::dark_mode_supported) return;

    Internal::theme = theme;

    using namespace Internal;

    if (_SetPreferredAppMode)
    {
        if (theme == Theme::System)
            _SetPreferredAppMode(AllowDark);
        else
            _SetPreferredAppMode(theme == Theme::Dark ? ForceDark : ForceLight);
    }

    const auto dark = is_dark();
    update_theme_data(dark);
    if (_AllowDarkModeForApp) _AllowDarkModeForApp(dark);
    _RefreshImmersiveColorPolicyState();
    if (_FlushMenuThemes) _FlushMenuThemes();
    patch_scrollbar(dark);

    for (const auto &[hwnd, exclude] : attached_windows)
    {
        update_window_theme(hwnd, dark, exclude);
    }
}

} // namespace WinDarkMode
