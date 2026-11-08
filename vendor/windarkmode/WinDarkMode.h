/*
 * Copyright (c) 2026, abart27 (https://github.com/abart27).
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * WinDarkMode - 2.0.0
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
#include <atomic>
#include <bit>
#include <cctype>
#include <optional>
#include <stdexcept>
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
 * @remarks Resources within are owned by WinDarkMode, don't delete or mutate them.
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

namespace Internal
{

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

} // namespace Internal

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
 * @brief Like `Theme`, but resolved to a specific theme.
 */
enum class EffectiveTheme
{
    /**
     * @brief The light theme.
     */
    Light,

    /**
     * @brief The dark theme.
     */
    Dark
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
using fnSetPreferredAppMode = PreferredAppMode(WINAPI *)(PreferredAppMode appMode); // ordinal 135, in 1903

inline fnFlushMenuThemes _FlushMenuThemes{};
inline fnSetWindowCompositionAttribute _SetWindowCompositionAttribute{};
inline fnShouldAppsUseDarkMode _ShouldAppsUseDarkMode{};
inline fnAllowDarkModeForWindow _AllowDarkModeForWindow{};
inline fnAllowDarkModeForApp _AllowDarkModeForApp{};
inline fnRefreshImmersiveColorPolicyState _RefreshImmersiveColorPolicyState{};
inline fnIsDarkModeAllowedForWindow _IsDarkModeAllowedForWindow{};
inline fnGetIsImmersiveColorUsingHighContrast _GetIsImmersiveColorUsingHighContrast{};
inline fnOpenNcThemeData _OpenNcThemeData{};
inline fnSetPreferredAppMode _SetPreferredAppMode{};

inline HMODULE h_uxtheme{};
inline ULONG_PTR original_open_nc_theme_data{};

inline std::atomic<DWORD> initialization_thread_id{0};
inline bool initialized = false;
inline Theme theme = Theme::System;
inline std::unordered_map<HWND, std::vector<HWND>> attached_windows;
inline std::unordered_set<HWND> pending_separator_repaint;
inline bool dark_mode_supported = false;
inline DWORD build_number = 0;

inline void require_initialization_thread()
{
    const DWORD thread_id = initialization_thread_id.load();
    if (!thread_id) throw std::logic_error("WinDarkMode::init() must be called first");
    if (thread_id != GetCurrentThreadId())
        throw std::logic_error("WinDarkMode must be used on the thread that called init()");
}

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

inline bool is_high_contrast()
{
    HIGHCONTRAST high_contrast{};
    high_contrast.cbSize = sizeof(high_contrast);
    if (!SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(high_contrast), &high_contrast, FALSE)) return false;
    return high_contrast.dwFlags & HCF_HIGHCONTRASTON;
}

inline EffectiveTheme effective_theme(Theme requested_theme = theme)
{
    if (requested_theme != Theme::System)
        return requested_theme == Theme::Dark ? EffectiveTheme::Dark : EffectiveTheme::Light;

    if (dark_mode_supported && _ShouldAppsUseDarkMode() && !is_high_contrast()) return EffectiveTheme::Dark;

    return EffectiveTheme::Light;
};

inline bool is_dark()
{
    return effective_theme() == EffectiveTheme::Dark;
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

inline void patch_scrollbar(bool dark)
{
    HMODULE comctl_mod = LoadLibraryEx(TEXT("comctl32.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!comctl_mod) return;

    PIMAGE_THUNK_DATA addr = nullptr;
    auto imports =
        data_directory_from_module_base<PIMAGE_DELAYLOAD_DESCRIPTOR>(comctl_mod, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
    for (; imports->DllNameRVA; ++imports)
    {
        if (!equal_ignore_case(rva_to_va<LPCSTR>(comctl_mod, imports->DllNameRVA), "uxtheme.dll")) continue;

        auto impName = rva_to_va<PIMAGE_THUNK_DATA>(comctl_mod, imports->ImportNameTableRVA);
        auto impAddr = rva_to_va<PIMAGE_THUNK_DATA>(comctl_mod, imports->ImportAddressTableRVA);
        for (; impName->u1.Ordinal; ++impName, ++impAddr)
        {
            if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal) &&
                IMAGE_ORDINAL(impName->u1.Ordinal) == 49) // OpenNcThemeData
            {
                addr = impAddr;
                break;
            }
        }
        break;
    }
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

struct DefaultControlThemer
{
    static void update(HWND hwnd, bool dark) { SetWindowTheme(hwnd, dark ? L"DarkMode_DarkTheme" : nullptr, nullptr); }
};

struct HeaderThemer
{
    static void update(HWND, bool) {}
};

struct TabControlThemer
{
    struct Context
    {
        int hover = -1;
    };

    static LRESULT CALLBACK subclass_proc(
        HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR dwRefData)
    {
        auto ctx = reinterpret_cast<Context *>(dwRefData);

        switch (msg)
        {
        case WM_NCDESTROY: {
            delete ctx;
            RemoveWindowSubclass(hwnd, subclass_proc, 0);
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

            for (int i = 0; i < nTabs; i++)
            {
                RECT rc_item{};
                TabCtrl_GetItemRect(hwnd, i, &rc_item);

                RECT rcIntersect{};
                if (!IntersectRect(&rcIntersect, &ps.rcPaint, &rc_item)) continue;

                const bool selected = (i == nSelTab);
                const bool hovered = (i == ctx->hover);
                HBRUSH tab_brush = selected ? theme_data.tab_normal_brush : theme_data.bg_brush;
                if (hovered && !selected) tab_brush = theme_data.tab_hover_brush;

                HPEN tab_pen = CreatePen(PS_SOLID, 1, theme_data.tab_hover_color);
                HPEN old_pen = static_cast<HPEN>(SelectObject(hdc, tab_pen));
                HBRUSH old_brush = static_cast<HBRUSH>(SelectObject(hdc, tab_brush));
                constexpr int corner_radius = 6;
                RoundRect(hdc, rc_item.left, rc_item.top, rc_item.right, rc_item.bottom, corner_radius, corner_radius);

                RECT bottom_rc = rc_item;
                bottom_rc.top = bottom_rc.bottom - corner_radius;
                FillRect(hdc, &bottom_rc, tab_brush);
                MoveToEx(hdc, rc_item.left, rc_item.top + corner_radius / 2, nullptr);
                LineTo(hdc, rc_item.left, rc_item.bottom);
                MoveToEx(hdc, rc_item.right - 1, rc_item.top + corner_radius / 2, nullptr);
                LineTo(hdc, rc_item.right - 1, rc_item.bottom);
                MoveToEx(hdc, rc_item.left, rc_item.bottom - 1, nullptr);
                LineTo(hdc, rc_item.right, rc_item.bottom - 1);

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
                DrawText(hdc, label, -1, &rc_item, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
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

    static void update(HWND hwnd, bool dark)
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
        if (GetWindowSubclass(hwnd, subclass_proc, 0, &old_data))
        {
            RemoveWindowSubclass(hwnd, subclass_proc, 0);
            delete reinterpret_cast<Context *>(old_data);
        }

        if (dark)
        {
            auto *ctx = new Context{};
            if (!SetWindowSubclass(hwnd, subclass_proc, 0, reinterpret_cast<DWORD_PTR>(ctx))) delete ctx;
        }

        InvalidateRect(hwnd, nullptr, TRUE);
    }
};

struct ListBoxThemer
{
    static LRESULT CALLBACK subclass_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        switch (msg)
        {
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, subclass_proc, 0);
            break;

        case WM_ERASEBKGND:
            return TRUE;
        case WM_PAINT:
        case WM_PRINTCLIENT: {
            PAINTSTRUCT ps{};
            HDC target = msg == WM_PAINT ? BeginPaint(hwnd, &ps) : reinterpret_cast<HDC>(wParam);
            RECT client{};
            GetClientRect(hwnd, &client);
            HDC buffer = CreateCompatibleDC(target);
            HBITMAP bitmap = buffer ? CreateCompatibleBitmap(target, client.right, client.bottom) : nullptr;
            HGDIOBJ old_bitmap = bitmap ? SelectObject(buffer, bitmap) : nullptr;
            HDC hdc = bitmap ? buffer : target;
            const int saved_dc = SaveDC(hdc);
            IntersectClipRect(hdc, client.left, client.top, client.right, client.bottom);
            FillRect(hdc, &client, theme_data.listbox_bg_brush);

            const auto style = GetWindowLongPtr(hwnd, GWL_STYLE);
            const bool multi_select = (style & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)) != 0;
            const bool enabled = IsWindowEnabled(hwnd) != FALSE;
            const bool focused = GetFocus() == hwnd;
            const int count = static_cast<int>(SendMessage(hwnd, LB_GETCOUNT, 0, 0));
            const int current = static_cast<int>(SendMessage(hwnd, LB_GETCURSEL, 0, 0));
            const int caret = static_cast<int>(SendMessage(hwnd, LB_GETCARETINDEX, 0, 0));
            const bool hide_focus = (SendMessage(hwnd, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS) != 0;
            HFONT font = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
            if (font) SelectObject(hdc, font);
            SetBkMode(hdc, TRANSPARENT);

            const int first = static_cast<int>(SendMessage(hwnd, LB_GETTOPINDEX, 0, 0));
            for (int i = first; i >= 0 && i < count; ++i)
            {
                RECT rc{};
                if (SendMessage(hwnd, LB_GETITEMRECT, i, reinterpret_cast<LPARAM>(&rc)) == LB_ERR) break;
                if ((style & LBS_MULTICOLUMN) ? rc.left >= client.right : rc.top >= client.bottom) break;
                RECT visible{};
                if (!IntersectRect(&visible, &client, &rc)) continue;

                const bool selected =
                    !(style & LBS_NOSEL) && (multi_select ? SendMessage(hwnd, LB_GETSEL, i, 0) > 0 : i == current);
                FillRect(hdc, &rc, selected ? theme_data.tab_normal_brush : theme_data.listbox_bg_brush);
                SetTextColor(hdc, enabled ? theme_data.text_1_color : theme_data.disabled_text_color);

                const LRESULT length = SendMessage(hwnd, LB_GETTEXTLEN, i, 0);
                if (length != LB_ERR)
                {
                    std::basic_string<TCHAR> text(static_cast<size_t>(length) + 1, TEXT('\0'));
                    if (SendMessage(hwnd, LB_GETTEXT, i, reinterpret_cast<LPARAM>(text.data())) != LB_ERR)
                    {
                        RECT text_rc = rc;
                        text_rc.left += 2;
                        DrawText(hdc, text.c_str(), -1, &text_rc,
                            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX |
                                ((style & LBS_USETABSTOPS) ? DT_EXPANDTABS : 0) |
                                ((GetWindowLongPtr(hwnd, GWL_EXSTYLE) & WS_EX_RTLREADING) ? DT_RTLREADING : 0));
                    }
                }
                if (focused && enabled && i == caret && !hide_focus) DrawFocusRect(hdc, &rc);
            }

            if (saved_dc) RestoreDC(hdc, saved_dc);
            if (bitmap)
            {
                BitBlt(target, 0, 0, client.right, client.bottom, buffer, 0, 0, SRCCOPY);
                SelectObject(buffer, old_bitmap);
                DeleteObject(bitmap);
            }
            if (buffer) DeleteDC(buffer);
            if (msg == WM_PAINT) EndPaint(hwnd, &ps);
            return 0;
        }
        default:
            break;
        }

        const LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        if ((msg >= LB_ADDSTRING && msg <= LB_GETLISTBOXINFO) || msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP ||
            msg == WM_LBUTTONDBLCLK || (msg == WM_MOUSEMOVE && GetCapture() == hwnd) || msg == WM_TIMER ||
            msg == WM_KEYDOWN || msg == WM_CHAR || msg == WM_SETFOCUS || msg == WM_KILLFOCUS || msg == WM_ENABLE ||
            msg == WM_SETFONT || msg == WM_SIZE || msg == WM_HSCROLL || msg == WM_VSCROLL || msg == WM_MOUSEWHEEL ||
            msg == WM_UPDATEUISTATE)
        {
            if (msg != LB_GETCOUNT && msg != LB_GETCURSEL && msg != LB_GETCARETINDEX && msg != LB_GETTOPINDEX &&
                msg != LB_GETITEMRECT && msg != LB_GETSEL && msg != LB_GETTEXTLEN && msg != LB_GETTEXT)
                InvalidateRect(hwnd, nullptr, FALSE);
        }
        return result;
    }

    static void update(HWND hwnd, bool dark)
    {
        const auto style = GetWindowLongPtr(hwnd, GWL_STYLE);
        if (style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE | LBS_NODATA)) return;

        constexpr LONG_PTR edge_mask = WS_EX_CLIENTEDGE | WS_EX_STATICEDGE;
        const auto ex_style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        DWORD_PTR original_frame = 0;
        const bool subclassed = GetWindowSubclass(hwnd, subclass_proc, 0, &original_frame) != FALSE;
        SetWindowTheme(hwnd, dark ? L"DarkMode_Explorer" : nullptr, nullptr);
        if (dark)
        {
            if (!subclassed)
            {
                original_frame = static_cast<DWORD_PTR>((style & WS_BORDER) | (ex_style & edge_mask));
                if (!SetWindowSubclass(hwnd, subclass_proc, 0, original_frame)) return;
            }
            SetWindowLongPtr(hwnd, GWL_STYLE, style & ~static_cast<LONG_PTR>(WS_BORDER));
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, ex_style & ~edge_mask);
        }
        else if (subclassed)
        {
            RemoveWindowSubclass(hwnd, subclass_proc, 0);
            SetWindowLongPtr(
                hwnd, GWL_STYLE, (style & ~static_cast<LONG_PTR>(WS_BORDER)) | (original_frame & WS_BORDER));
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, (ex_style & ~edge_mask) | (original_frame & edge_mask));
        }
        SetWindowPos(
            hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME);
    }
};

struct ListViewThemer
{
    struct Context
    {
        COLORREF hdr_text_color;
    };

    static LRESULT CALLBACK subclass_proc(
        HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR dwRefData)
    {
        auto info = reinterpret_cast<Context *>(dwRefData);

        switch (msg)
        {
        case WM_NCDESTROY: {
            delete info;
            RemoveWindowSubclass(hwnd, subclass_proc, 0);
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

    static void update(HWND lv_hwnd, bool dark)
    {
        HWND hdr_hwnd = ListView_GetHeader(lv_hwnd);
        _AllowDarkModeForWindow(hdr_hwnd, dark);

        DWORD_PTR old_data = 0;
        if (GetWindowSubclass(lv_hwnd, subclass_proc, 0, &old_data)) delete reinterpret_cast<Context *>(old_data);

        Context *ctx = nullptr;
        if (dark)
        {
            ctx = new Context{};
            SetWindowSubclass(lv_hwnd, subclass_proc, 0, reinterpret_cast<DWORD_PTR>(ctx));

            // FIXME: We force grid lines off because they look absolutely brutal in dark mode. Maybe we can override
            // them?
            const auto ex_style = ListView_GetExtendedListViewStyle(lv_hwnd);
            ListView_SetExtendedListViewStyle(lv_hwnd, ex_style & ~LVS_EX_GRIDLINES);

            // FIXME: Hide focus rectangle because it's white :/ Would be nice to override it instead.
            SendMessage(lv_hwnd, WM_CHANGEUISTATE, MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);

            SetWindowTheme(hdr_hwnd, L"ItemsView", L"Header");
            SetWindowTheme(lv_hwnd, L"ItemsView", 0);
        }
        else
        {
            RemoveWindowSubclass(lv_hwnd, subclass_proc, 0);
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
};

struct GroupBoxThemer
{
    static LRESULT CALLBACK subclass_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId, DWORD_PTR)
    {
        switch (msg)
        {
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, subclass_proc, sId);
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
                RECT clear_rc = {
                    text_x - text_padding, rc.top, text_x + text_size.cx + text_padding, rc.top + tm.tmHeight};
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

    static void update(HWND hwnd, bool dark)
    {
        if (dark)
            SetWindowSubclass(hwnd, subclass_proc, 0, 0);
        else
            RemoveWindowSubclass(hwnd, subclass_proc, 0);
    }
};

struct ButtonThemer
{
    struct Context
    {
        bool hot = false;
    };

    static LRESULT CALLBACK subclass_proc(
        HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId, DWORD_PTR dwRefData)
    {
        auto *ctx = reinterpret_cast<Context *>(dwRefData);
        switch (msg)
        {
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, subclass_proc, sId);
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

    static void update(HWND hwnd, bool dark)
    {
        const auto style = GetWindowLongPtr(hwnd, GWL_STYLE) & 0xFL;
        if (style == BS_GROUPBOX)
        {
            GroupBoxThemer::update(hwnd, dark);
            return;
        }

        const bool is_check_or_radio = style == BS_CHECKBOX || style == BS_AUTOCHECKBOX || style == BS_3STATE ||
                                       style == BS_AUTO3STATE || style == BS_RADIOBUTTON || style == BS_AUTORADIOBUTTON;
        if (is_check_or_radio)
        {
            DWORD_PTR old_data = 0;
            if (GetWindowSubclass(hwnd, subclass_proc, 0, &old_data))
            {
                RemoveWindowSubclass(hwnd, subclass_proc, 0);
                delete reinterpret_cast<Context *>(old_data);
            }

            SetWindowTheme(hwnd, dark ? L"DarkMode_DarkTheme" : nullptr, nullptr);
            if (dark) SetWindowSubclass(hwnd, subclass_proc, 0, reinterpret_cast<DWORD_PTR>(new Context{}));

            InvalidateRect(hwnd, nullptr, TRUE);
            return;
        }

        DefaultControlThemer::update(hwnd, dark);
    }
};

struct StatusBarThemer
{
    static LRESULT CALLBACK subclass_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId, DWORD_PTR)
    {
        switch (msg)
        {
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, subclass_proc, sId);
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

    static void update(HWND hwnd, bool dark)
    {
        if (dark)
        {
            SetWindowTheme(hwnd, L"", L"");
            SetWindowSubclass(hwnd, subclass_proc, 0, 0);
        }
        else
        {
            RemoveWindowSubclass(hwnd, subclass_proc, 0);
            SetWindowTheme(hwnd, nullptr, nullptr);
        }
    }
};

struct ControlThemer
{
    LPCTSTR class_name;
    void (*update)(HWND hwnd, bool dark);
};

inline void update_control(HWND hwnd, bool dark, const std::vector<HWND> &exclude)
{
    TCHAR cls[32]{};
    GetClassName(hwnd, cls, std::size(cls));
    const std::basic_string<TCHAR> class_name(cls);

    if (std::find(exclude.begin(), exclude.end(), hwnd) != exclude.end()) return;

    _AllowDarkModeForWindow(hwnd, dark);

    static constexpr ControlThemer themers[] = {
        {WC_HEADER, HeaderThemer::update},
        {WC_LISTVIEW, ListViewThemer::update},
        {WC_LISTBOX, ListBoxThemer::update},
        {WC_TABCONTROL, TabControlThemer::update},
        {WC_BUTTON, ButtonThemer::update},
        {STATUSCLASSNAME, StatusBarThemer::update},
    };
    for (const auto &themer : themers)
    {
        if (class_name == themer.class_name)
        {
            themer.update(hwnd, dark);
            return;
        }
    }

    DefaultControlThemer::update(hwnd, dark);
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
    case WM_SETTINGCHANGE: {
        if (theme != Theme::System) break;

        const auto lparam_str = reinterpret_cast<LPCTSTR>(lParam);
        const bool theme_changed = lParam && lstrcmpi(lparam_str, TEXT("ImmersiveColorSet")) == 0;
        if (theme_changed) _RefreshImmersiveColorPolicyState();
        _GetIsImmersiveColorUsingHighContrast(IHCM_REFRESH);
        if (theme_changed)
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
    }
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

inline void update_window_theme(HWND hwnd, bool dark, const std::vector<HWND> &exclude)
{
    _AllowDarkModeForWindow(hwnd, dark);
    BOOL dark2 = dark;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark2, sizeof(dark2));
    if (build_number < 18362)
        SetProp(hwnd, TEXT("UseImmersiveDarkModeColors"), reinterpret_cast<HANDLE>(static_cast<INT_PTR>(dark)));
    if (_SetWindowCompositionAttribute)
    {
        WINDOWCOMPOSITIONATTRIBDATA data = {WCA_USEDARKMODECOLORS, &dark2, sizeof(dark2)};
        _SetWindowCompositionAttribute(hwnd, &data);
    }

    struct UpdateChildrenContext
    {
        bool dark;
        const std::vector<HWND> *exclude;
    };
    UpdateChildrenContext ctx{dark, &exclude};
    EnumChildWindows(
        hwnd,
        [](HWND hwnd, LPARAM lparam) -> BOOL {
            const auto *ctx = reinterpret_cast<const UpdateChildrenContext *>(lparam);
            update_control(hwnd, ctx->dark, *ctx->exclude);
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&ctx));
    DrawMenuBar(hwnd);

    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

inline void update_theme_data(ThemeData &data, bool dark)
{
#define RECREATE(x, color)                                                                                             \
    if (data.x##_brush)                                                                                                \
    {                                                                                                                  \
        DeleteObject(data.x##_brush);                                                                                  \
        data.x##_brush = nullptr;                                                                                      \
    }                                                                                                                  \
    data.x##_brush = CreateSolidBrush(color);                                                                          \
    data.x##_color = color;

    const auto bg_color = dark ? dark_theme_data.bg_color : GetSysColor(COLOR_3DFACE);
    RECREATE(bg, bg_color)
    RECREATE(text_1, dark ? dark_theme_data.text_1_color : light_theme_data.text_1_color)
    RECREATE(text_2, dark ? dark_theme_data.text_2_color : light_theme_data.text_2_color)
    RECREATE(listbox_bg, dark ? dark_theme_data.listbox_bg_color : light_theme_data.listbox_bg_color)
    RECREATE(edit_bg, dark ? dark_theme_data.edit_bg_color : light_theme_data.edit_bg_color)
    RECREATE(tab_normal, dark ? dark_theme_data.tab_normal_color : light_theme_data.tab_normal_color)
    RECREATE(tab_hover, dark ? dark_theme_data.tab_hover_color : light_theme_data.tab_hover_color)
    RECREATE(disabled_text, dark ? dark_theme_data.disabled_text_color : light_theme_data.disabled_text_color)

#define UPDATE_COLOR(x) data.x##_color = dark ? dark_theme_data.x##_color : light_theme_data.x##_color;
    UPDATE_COLOR(groupbox_border)
    UPDATE_COLOR(statusbar_border)
    UPDATE_COLOR(statusbar_divider)
    UPDATE_COLOR(statusbar_grip)
#undef UPDATE_COLOR
#undef RECREATE
}

inline void update_theme_data(bool dark)
{
    update_theme_data(theme_data, dark);
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
 * @brief Options for `init`.
 */
struct InitOptions
{
    /**
     * @brief Whether to automatically theme messageboxes. Defaults to `true`.
     * @remarks Only applies to dialogs owned by the thread that first called `init()`.
     */
    std::optional<bool> affect_messageboxes = std::nullopt;
};

namespace Internal
{
inline InitOptions init_options;
inline HWINEVENTHOOK dialog_creation_hook = nullptr;

inline void CALLBACK dialog_creation_event(
    HWINEVENTHOOK, DWORD event, HWND hwnd, LONG object_id, LONG child_id, DWORD, DWORD);
} // namespace Internal

/**
 * @brief Initializes dark mode support and binds the library to the calling UI thread.
 * @param options Options for automatic dialog attachment.
 * @throws std::logic_error If called from a thread other than the one that first called `init()`.
 * @remarks
 * This function does nothing if WinDarkMode is already initialized.
 *
 * Call this before any other public function.
 * All public function calls after initialization must stay on this thread.
 *
 * Windows passed to attach() must belong to this thread and process.
 *
 */
inline void init(const InitOptions &options = {})
{
    using namespace Internal;

    DWORD expected_thread_id = 0;
    initialization_thread_id.compare_exchange_strong(expected_thread_id, GetCurrentThreadId());
    require_initialization_thread();
    if (initialized) return;

    initialized = true;
    update_theme_data(false);

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
    }

    WinDarkMode::set(theme);

    init_options = options;
    if (dark_mode_supported && options.affect_messageboxes.value_or(true))
    {
        dialog_creation_hook = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_SHOW, nullptr, dialog_creation_event,
            GetCurrentProcessId(), GetCurrentThreadId(), WINEVENT_OUTOFCONTEXT);
    }
}

/**
 * @brief Attaches dark mode support to a window. If the window has already been attached to, no work will be done.
 * @param hwnd The handle to the window.
 * @param options Options for attaching. See `AttachOptions` for details.
 * @remarks Adding, changing, or removing child windows after this call is not properly supported yet.
 * @throws std::logic_error If called from a thread other than the one that first called `init()`.
 */
inline void attach(HWND hwnd, const AttachOptions &options = {})
{
    using namespace Internal;

    require_initialization_thread();
    DWORD process_id = 0;
    const DWORD thread_id = GetWindowThreadProcessId(hwnd, &process_id);
    if (thread_id != GetCurrentThreadId() || process_id != GetCurrentProcessId())
        throw std::logic_error(
            "WinDarkMode::attach() requires a valid window owned by the thread that called WinDarkMode::init()");

    if (!dark_mode_supported || attached_windows.contains(hwnd)) return;

    attached_windows.emplace(hwnd, options.exclude);

    const auto dark = is_dark();
    update_theme_data(dark);
    if (_FlushMenuThemes) _FlushMenuThemes();
    patch_scrollbar(dark);
    SetWindowSubclass(hwnd, wnd_subclass_proc, 0, 0);

    bool top_level = false;
    if (IsWindow(hwnd))
    {
        const auto style = GetWindowLongPtr(hwnd, GWL_STYLE);
        if ((style & WS_OVERLAPPEDWINDOW) || (style & WS_POPUP)) top_level = !GetParent(hwnd);
    }
    const bool is_dialog = options.is_dialog.value_or(!top_level);
    if (is_dialog) SetWindowSubclass(hwnd, dlg_subclass_proc, 0, 0);

    update_window_theme(hwnd, dark, options.exclude);
}

namespace Internal
{
inline void paint_messagebox(HWND hwnd, HDC hdc, bool exclude_children = true)
{
    const int saved = SaveDC(hdc);
    if (!saved) return;
    for (HWND child = exclude_children ? GetWindow(hwnd, GW_CHILD) : nullptr; child;
        child = GetWindow(child, GW_HWNDNEXT))
    {
        if (!IsWindowVisible(child)) continue;
        RECT rc{};
        GetWindowRect(child, &rc);
        MapWindowPoints(nullptr, hwnd, reinterpret_cast<POINT *>(&rc), 2);
        ExcludeClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
    }
    RECT client{};
    GetClientRect(hwnd, &client);
    FillRect(hdc, &client, theme_data.bg_brush);
    RECT band{};
    GetClientRect(hwnd, &band);
    LONG button_top = band.bottom;
    LONG button_bottom = band.bottom;
    for (HWND child = GetWindow(hwnd, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT))
    {
        WCHAR class_name[32]{};
        GetClassNameW(child, class_name, 32);
        if (!IsWindowVisible(child) || lstrcmpiW(class_name, L"Button") != 0) continue;
        const auto type = GetWindowLongPtr(child, GWL_STYLE) & BS_TYPEMASK;
        if (type != BS_PUSHBUTTON && type != BS_DEFPUSHBUTTON) continue;
        RECT button{};
        GetWindowRect(child, &button);
        MapWindowPoints(nullptr, hwnd, reinterpret_cast<POINT *>(&button), 2);
        button_top = (std::min)(button_top, button.top);
        button_bottom = button.bottom;
    }
    band.top = (std::max)(0L, button_top - (band.bottom - button_bottom));
    FillRect(hdc, &band, theme_data.tab_normal_brush);
    RestoreDC(hdc, saved);
}

inline LRESULT CALLBACK messagebox_subclass_proc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR)
{
    if (msg == WM_NCDESTROY)
        RemoveWindowSubclass(hwnd, messagebox_subclass_proc, id);
    else if (is_dark() && !is_high_contrast())
    {
        switch (msg)
        {
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            const HDC hdc = BeginPaint(hwnd, &ps);
            if (hdc) paint_messagebox(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_PRINTCLIENT:
        case WM_ERASEBKGND:
            paint_messagebox(hwnd, reinterpret_cast<HDC>(wParam), false);
            return TRUE;
        case WM_CTLCOLORBTN:
            SetTextColor(reinterpret_cast<HDC>(wParam), theme_data.text_1_color);
            SetBkColor(reinterpret_cast<HDC>(wParam), theme_data.tab_normal_color);
            return reinterpret_cast<LRESULT>(theme_data.tab_normal_brush);
        default:
            break;
        }
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

inline void CALLBACK dialog_creation_event(
    HWINEVENTHOOK, DWORD event, HWND hwnd, LONG object_id, LONG child_id, DWORD, DWORD)
{
    if ((event != EVENT_OBJECT_CREATE && event != EVENT_OBJECT_SHOW) || !hwnd || object_id != OBJID_WINDOW ||
        child_id != CHILDID_SELF)
        return;

    DWORD process_id = 0;
    const DWORD thread_id = GetWindowThreadProcessId(hwnd, &process_id);
    if (process_id != GetCurrentProcessId() || thread_id != initialization_thread_id.load() ||
        GetCurrentThreadId() != thread_id)
        return;

    if (attached_windows.contains(hwnd) || (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CHILD)) return;

    WCHAR class_name[32] = {};
    if (!GetClassNameW(hwnd, class_name, 32) || std::wstring(class_name) != L"#32770") return;

    const auto dialog_proc = GetWindowLongPtrW(hwnd, DWLP_DLGPROC);
    HMODULE module = nullptr;
    if (!dialog_proc ||
        !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(dialog_proc), &module))
        return;

    WCHAR path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(module, path, MAX_PATH);
    if (!length || length >= MAX_PATH) return;
    const std::wstring module_path(path, length);
    const auto name = module_path.substr(module_path.find_last_of(L"\\/") + 1);
    const bool messagebox = lstrcmpiW(name.c_str(), L"user32.dll") == 0;
    if (messagebox && init_options.affect_messageboxes.value_or(true) && is_dark() && !is_high_contrast())
    {
        // HACK: MessageBoxes have this white band at the bottom drawn in their WM_PAINT which we need to override :/
        WinDarkMode::attach(hwnd, {.is_dialog = true, .exclude = {}});
        SetWindowSubclass(hwnd, messagebox_subclass_proc, 0, 0);
        InvalidateRect(hwnd, nullptr, TRUE);
    }
}
} // namespace Internal

/**
 * @brief Sets the app theme.
 * @param theme The theme to set.
 * @throws std::logic_error If called from a thread other than the one that first called `init()`.
 */
inline void set(Theme theme)
{
    Internal::require_initialization_thread();
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

/**
 * @brief Gets the effective theme.
 * @throws std::logic_error If called from a thread other than the one that first called `init()`.
 */
inline EffectiveTheme effective_theme()
{
    using namespace Internal;
    require_initialization_thread();
    if (!dark_mode_supported) return EffectiveTheme::Light;
    return is_dark() ? EffectiveTheme::Dark : EffectiveTheme::Light;
}

/**
 * @brief Gets the current theme data. Don't use the reference off-thread.
 * @throws std::logic_error If called before `init()` or from a different thread.
 */
inline const ThemeData &theme_data()
{
    Internal::require_initialization_thread();
    return Internal::theme_data;
}

/**
 * @brief Gets the theme data for the specified theme. Don't use the reference off-thread.
 * @throws std::logic_error If called before `init()` or from a different thread.
 */
inline const ThemeData &theme_data(Theme theme)
{
    Internal::require_initialization_thread();
    const bool dark = Internal::effective_theme(theme) == EffectiveTheme::Dark;
    static ThemeData light_data = Internal::light_theme_data;
    static ThemeData dark_data = Internal::dark_theme_data;
    auto &data = dark ? dark_data : light_data;
    if (!data.bg_brush || (!dark && data.bg_color != GetSysColor(COLOR_3DFACE)))
        Internal::update_theme_data(data, dark);
    return data;
}

} // namespace WinDarkMode
