/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "ConfigDialog_Win32.hpp"
#include "Config.hpp"
#include "glN64.hpp"
#include "Resource.h"
#include "GBI.hpp"
#include "OpenGL.hpp"
#include "RSP.hpp"
#include "Textures.h"
#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>
#include <nlohmann/json.hpp>
using nlohmann::json;

static void apply(HWND hWndDlg)
{
    static GLInfo prev_OGL{};

    prev_OGL = OGL;

    char text[256]{};
    int i;

    Edit_GetText(GetDlgItem(hWndDlg, IDC_CACHEMEGS), text, 4);
    cache.maxBytes = atol(text) * 1048576;

    OGL.textureFilter = (TextureFilter)SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_GETCURSEL, 0, 0);
    OGL.filterScale = SendDlgItemMessage(hWndDlg, IDC_FSCALE, TBM_GETPOS, 0, 0);
    OGL.fog = (SendDlgItemMessage(hWndDlg, IDC_FOG, BM_GETCHECK, 0, 0) == BST_CHECKED);
    OGL.msaa = (SendDlgItemMessage(hWndDlg, IDC_MSAA, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 4 : 0;
    OGL.originAdjust = (OGL.textureFilter == TextureFilter::SaI ? 0.25 : 0.50);
    OGL.ignoreScissor = (SendDlgItemMessage(hWndDlg, IDC_SCISSOR, BM_GETCHECK, 0, 0) == BST_CHECKED);
    OGL.clear_override = (SendDlgItemMessage(hWndDlg, IDC_CLEAR, BM_GETCHECK, 0, 0) == BST_CHECKED);

    char val[32]{};
    SendMessage(GetDlgItem(hWndDlg, IDC_WINDOWED_X), WM_GETTEXT, std::size(val), (LPARAM)val);
    OGL.windowedWidth = atoi(val);
    SendMessage(GetDlgItem(hWndDlg, IDC_WINDOWED_Y), WM_GETTEXT, std::size(val), (LPARAM)val);
    OGL.windowedHeight = atoi(val);

    OGL.smoothing = ComboBox_GetCurSel(GetDlgItem(hWndDlg, IDC_SMOOTHING));
    OGL.aspectMode = to_aspect_mode(ComboBox_GetCurSel(GetDlgItem(hWndDlg, IDC_ASPECT)));

    OGL.usePolygonStipple = (SendDlgItemMessage(hWndDlg, IDC_DITHEREDALPHATEST, BM_GETCHECK, 0, 0) == BST_CHECKED);

    Config_SaveConfig();
    Config_LoadConfig();

    const auto needs_restart =
        OGL.smoothing != prev_OGL.smoothing || OGL.textureFilter != prev_OGL.textureFilter ||
        OGL.filterScale != prev_OGL.filterScale || OGL.msaa != prev_OGL.msaa ||
        OGL.ignoreScissor != prev_OGL.ignoreScissor || OGL.clear_override != prev_OGL.clear_override ||
        OGL.windowedWidth != prev_OGL.windowedWidth || OGL.windowedHeight != prev_OGL.windowedHeight ||
        OGL.usePolygonStipple != prev_OGL.usePolygonStipple;

    if (RSP.thread && needs_restart)
    {
        SetEvent(RSP.threadMsg[RSPMSG_RESTART]);
        WaitForSingleObject(RSP.threadFinished, INFINITE);
    }
}

static void select_resolution_in_combobox(HWND cb_hwnd, uint32_t width, uint32_t height)
{
    for (size_t i = 0; i < RESOLUTION_PRESETS.size(); i++)
    {
        const auto &preset = RESOLUTION_PRESETS[i];
        if (width == preset.width && height == preset.height)
        {
            ComboBox_SetCurSel(cb_hwnd, i);
            return;
        }
    }

    ComboBox_SetCurSel(cb_hwnd, -1);
}

BOOL CALLBACK ConfigDlgProc(HWND hWndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG: {
        const auto cb_hwnd = GetDlgItem(hWndDlg, IDC_WINDOWEDRES);

        for (const auto &preset : RESOLUTION_PRESETS)
        {
            ComboBox_AddString(cb_hwnd, preset.description.c_str());
        }

        for (const auto &filter : FILTER_NAMES)
        {
            ComboBox_AddString(GetDlgItem(hWndDlg, IDC_SMOOTHING), filter.second.c_str());
        }
        ComboBox_SetCurSel(GetDlgItem(hWndDlg, IDC_SMOOTHING), (int)OGL.smoothing);

        for (const auto &aspect_mode : ASPECT_MODE_NAMES)
        {
            ComboBox_AddString(GetDlgItem(hWndDlg, IDC_ASPECT), aspect_mode.c_str());
        }
        ComboBox_SetCurSel(GetDlgItem(hWndDlg, IDC_ASPECT), (int)OGL.aspectMode);

        select_resolution_in_combobox(cb_hwnd, OGL.windowedWidth, OGL.windowedHeight);

        SendDlgItemMessage(hWndDlg, IDC_WINDOWED_X, WM_SETTEXT, 0, (LPARAM)std::to_string(OGL.windowedWidth).c_str());
        SendDlgItemMessage(hWndDlg, IDC_WINDOWED_Y, WM_SETTEXT, 0, (LPARAM)std::to_string(OGL.windowedHeight).c_str());

        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_ADDSTRING, 0, (LPARAM) "None");
        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_ADDSTRING, 0, (LPARAM) "2xSaI");
        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_ADDSTRING, 0, (LPARAM) "xBRZ");
        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_ADDSTRING, 0, (LPARAM) "Hqx");
        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_SETCURSEL, (int)OGL.textureFilter, 0);
        SendMessage(GetDlgItem(hWndDlg, IDC_FSCALE), TBM_SETPOS, TRUE, OGL.filterScale);

        SendDlgItemMessage(hWndDlg, IDC_SCISSOR, BM_SETCHECK,
                           OGL.ignoreScissor ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED, 0);
        SendDlgItemMessage(hWndDlg, IDC_CLEAR, BM_SETCHECK,
                           OGL.clear_override ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED, 0);

        // Enable/disable fog
        SendDlgItemMessage(hWndDlg, IDC_FOG, BM_SETCHECK, OGL.fog ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED, 0);
        SendDlgItemMessage(hWndDlg, IDC_MSAA, BM_SETCHECK, OGL.msaa == 4 ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED,
                           0);

        SendDlgItemMessage(hWndDlg, IDC_DITHEREDALPHATEST, BM_SETCHECK,
                           OGL.usePolygonStipple ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED, 0);

        const auto cache_size = std::to_string(cache.maxBytes / 1048576);
        SendDlgItemMessage(hWndDlg, IDC_CACHEMEGS, WM_SETTEXT, 0, (LPARAM)cache_size.c_str());

        SendMessage(hWndDlg, WM_COMMAND, MAKEWPARAM(IDC_TEXTUREFILTER, CBN_SELCHANGE),
                    (LPARAM)GetDlgItem(hWndDlg, IDC_TEXTUREFILTER));

        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            apply(hWndDlg);
            EndDialog(hWndDlg, wParam);
            return TRUE;
        case IDCANCEL:
            EndDialog(hWndDlg, wParam);
            return TRUE;
        case IDC_WINDOWEDRES:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                const auto cb_hwnd = (HWND)lParam;
                const auto index = ComboBox_GetCurSel(cb_hwnd);

                const auto &preset = RESOLUTION_PRESETS[index];
                Edit_SetText(GetDlgItem(hWndDlg, IDC_WINDOWED_X), std::to_string(preset.width).c_str());
                Edit_SetText(GetDlgItem(hWndDlg, IDC_WINDOWED_Y), std::to_string(preset.height).c_str());
            }
            break;
        case IDC_WINDOWED_X:
        case IDC_WINDOWED_Y:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                char w_str[32]{};
                char h_str[32]{};
                Edit_GetText(GetDlgItem(hWndDlg, IDC_WINDOWED_X), w_str, std::size(w_str));
                Edit_GetText(GetDlgItem(hWndDlg, IDC_WINDOWED_Y), h_str, std::size(h_str));

                uint32_t width{};
                uint32_t height{};
                try
                {
                    width = std::stoul(w_str);
                    height = std::stoul(h_str);
                }
                catch (...)
                {
                    break;
                }

                select_resolution_in_combobox(GetDlgItem(hWndDlg, IDC_WINDOWEDRES), width, height);
            }
            break;
        case IDC_TEXTUREFILTER:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                const auto filter = (TextureFilter)SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_GETCURSEL, 0, 0);

                int min_scale = 0;
                int max_scale = 0;
                switch (filter)
                {
                case TextureFilter::SaI:
                    min_scale = max_scale = 2;
                    break;
                case TextureFilter::xBRZ:
                    min_scale = 2;
                    max_scale = 6;
                    break;
                case TextureFilter::Hqx:
                    min_scale = 2;
                    max_scale = 4;
                    break;
                default:
                    break;
                }

                SendMessage(GetDlgItem(hWndDlg, IDC_FSCALE), TBM_SETRANGE, TRUE, MAKELONG(min_scale, max_scale));
                EnableWindow(GetDlgItem(hWndDlg, IDC_FSCALE), min_scale != max_scale);
            }
        }
    }
    return FALSE;
}

void TASVideo::ConfigDialog::show(HWND parent)
{
    DialogBox(g_tas_ctx.hinst, MAKEINTRESOURCE(IDD_CONFIGDLG), parent, (DLGPROC)ConfigDlgProc);
}
