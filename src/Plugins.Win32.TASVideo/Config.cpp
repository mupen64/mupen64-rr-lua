/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "Config.hpp"
#include "glN64.hpp"
#include "resource.h"
#include "RSP.hpp"
#include "Textures.h"
#include "OpenGL.hpp"

#include <nlohmann/json.hpp>

HWND hConfigDlg;

#define numWindowedModes 12
#define CONFIG_FILE_NAME "TASVideo.json"

using nlohmann::json;

struct ResolutionPreset
{
    uint32_t width{};
    uint32_t height{};
    std::wstring description;
};

const std::vector<ResolutionPreset> RESOLUTION_PRESETS = {
    {320, 240, L"320 x 240 (4:3)"},     {400, 300, L"400 x 300 (4:3)"},      {480, 360, L"480 x 360 (4:3)"},
    {640, 480, L"640 x 480 (4:3)"},     {800, 600, L"800 x 600 (4:3)"},      {960, 720, L"960 x 720 (4:3)"},
    {1024, 768, L"1024 x 768 (4:3)"},   {1152, 864, L"1152 x 864 (4:3)"},    {1280, 720, L"1280 x 720 (16:9)"},
    {1280, 960, L"1280 x 960 (4:3)"},   {1280, 1024, L"1280 x 1024 (5:4)"},  {1440, 1080, L"1440 x 1080 (4:3)"},
    {1600, 1200, L"1600 x 1200 (4:3)"}, {1920, 1080, L"1920 x 1080 (16:9)"}, {2560, 1440, L"2560 x 1440 (16:9)"},
    {3840, 2160, L"3840 x 2160 (16:9)"}};

const std::vector<std::pair<uint8_t, std::wstring>> FILTER_NAMES = {
    {0, L"Default"},
    {1, L"Always Smooth"},
    {2, L"Always Pixelated"},
};

static std::optional<ResolutionPreset> get_preset_by_resolution(uint32_t width, uint32_t height)
{
    for (const auto &preset : RESOLUTION_PRESETS)
    {
        if (preset.width == width && preset.height == height) return preset;
    }
    return std::nullopt;
}

static std::filesystem::path get_config_path()
{
    const auto size = g_plugin->config_path(nullptr, 0);
    std::string path(size - 1, '\0');
    g_plugin->config_path(path.data(), size);
    return std::filesystem::path(path) / CONFIG_FILE_NAME;
}

static void Config_SetDefaults()
{
    OGL.smoothing = 0;
    OGL.fog = TRUE;
    OGL.msaa = 0;
    OGL.windowedWidth = 640;
    OGL.windowedHeight = 480;
    cache.maxBytes = 32 * 1048576;
    OGL.textureFilter = TextureFilter::None;
    OGL.usePolygonStipple = FALSE;
}

void Config_LoadConfig()
{
    auto json_path = get_config_path();

    if (!std::filesystem::exists(json_path))
    {
        Config_SetDefaults();
        Config_SaveConfig();
        return;
    }

    std::ifstream ifs(json_path);
    nlohmann::json j;

    try
    {
        ifs >> j;
        OGL.windowedWidth = j["windowed_width"];
        OGL.windowedHeight = j["windowed_height"];
        OGL.smoothing = j["smoothing"];
        OGL.textureFilter = j["texture_filter"];
        OGL.filterScale = j["filter_scale"];
        OGL.fog = j["enable_fog"];
        OGL.msaa = (j.value("msaa", 0) == 4) ? 4 : 0;
        cache.maxBytes = (int)j["texture_cache_size"] * 1048576;
        OGL.usePolygonStipple = j["dithered_alpha_testing"];
        OGL.ignoreScissor = j["ignore_scissor"];
        OGL.clear_override = j["clear_override"];
    }
    catch (const std::exception &e)
    {
        g_plugin->log_warn(L"Config load failed, using defaults...");
        Config_SetDefaults();
    }
}

void Config_SaveConfig()
{
    json j = json::object({
        {"windowed_width", OGL.windowedWidth},
        {"windowed_height", OGL.windowedHeight},
        {"smoothing", OGL.smoothing},
        {"texture_filter", OGL.textureFilter},
        {"filter_scale", OGL.filterScale},
        {"enable_fog", (bool)OGL.fog},
        {"msaa", OGL.msaa},
        {"texture_cache_size", cache.maxBytes / 1048576},
        {"dithered_alpha_testing", (bool)OGL.usePolygonStipple},
        {"ignore_scissor", (bool)OGL.ignoreScissor},
        {"clear_override", (bool)OGL.clear_override},
    });

    std::ofstream ofs(get_config_path());
    ofs << std::setw(2) << j;
}

void Config_ApplyDlgConfig(HWND hWndDlg)
{
    static GLInfo prev_OGL{};

    prev_OGL = OGL;

    wchar_t text[256]{};
    int i;

    Edit_GetText(GetDlgItem(hWndDlg, IDC_CACHEMEGS), text, 4);
    cache.maxBytes = _wtol(text) * 1048576;

    OGL.textureFilter = (TextureFilter)SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_GETCURSEL, NULL, NULL);
    OGL.filterScale = SendDlgItemMessage(hWndDlg, IDC_FSCALE, TBM_GETPOS, NULL, NULL);
    OGL.fog = (SendDlgItemMessage(hWndDlg, IDC_FOG, BM_GETCHECK, NULL, NULL) == BST_CHECKED);
    OGL.msaa = (SendDlgItemMessage(hWndDlg, IDC_MSAA, BM_GETCHECK, NULL, NULL) == BST_CHECKED) ? 4 : 0;
    OGL.originAdjust = (OGL.textureFilter == TextureFilter::SaI ? 0.25 : 0.50);
    OGL.ignoreScissor = (SendDlgItemMessage(hWndDlg, IDC_SCISSOR, BM_GETCHECK, NULL, NULL) == BST_CHECKED);
    OGL.clear_override = (SendDlgItemMessage(hWndDlg, IDC_CLEAR, BM_GETCHECK, NULL, NULL) == BST_CHECKED);

    wchar_t val[32]{};
    SendMessage(GetDlgItem(hWndDlg, IDC_WINDOWED_X), WM_GETTEXT, std::size(val), (LPARAM)val);
    OGL.windowedWidth = _wtoi(val);
    SendMessage(GetDlgItem(hWndDlg, IDC_WINDOWED_Y), WM_GETTEXT, std::size(val), (LPARAM)val);
    OGL.windowedHeight = _wtoi(val);

    OGL.smoothing = ComboBox_GetCurSel(GetDlgItem(hWndDlg, IDC_SMOOTHING));

    OGL.usePolygonStipple =
        (SendDlgItemMessage(hWndDlg, IDC_DITHEREDALPHATEST, BM_GETCHECK, NULL, NULL) == BST_CHECKED);

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

static void select_current_resolution_in_combobox(HWND cb_hwnd)
{
    for (size_t i = 0; i < RESOLUTION_PRESETS.size(); i++)
    {
        const auto &preset = RESOLUTION_PRESETS[i];
        if (OGL.windowedWidth == preset.width && OGL.windowedHeight == preset.height)
        {
            ComboBox_SetCurSel(cb_hwnd, i);
            return;
        }
    }
}

BOOL CALLBACK ConfigDlgProc(HWND hWndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG: {
        hConfigDlg = hWndDlg;

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

        select_current_resolution_in_combobox(cb_hwnd);

        SendDlgItemMessage(hWndDlg, IDC_WINDOWED_X, WM_SETTEXT, 0, (LPARAM)std::to_wstring(OGL.windowedWidth).c_str());
        SendDlgItemMessage(hWndDlg, IDC_WINDOWED_Y, WM_SETTEXT, 0, (LPARAM)std::to_wstring(OGL.windowedHeight).c_str());

        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_ADDSTRING, 0, (LPARAM)L"None");
        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_ADDSTRING, 0, (LPARAM)L"2xSaI");
        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_ADDSTRING, 0, (LPARAM)L"xBRZ");
        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_ADDSTRING, 0, (LPARAM)L"Hqx");
        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_SETCURSEL, (int)OGL.textureFilter, 0);
        SendMessage(GetDlgItem(hWndDlg, IDC_FSCALE), TBM_SETPOS, TRUE, OGL.filterScale);

        SendDlgItemMessage(hWndDlg, IDC_SCISSOR, BM_SETCHECK,
                           OGL.ignoreScissor ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED, NULL);
        SendDlgItemMessage(hWndDlg, IDC_CLEAR, BM_SETCHECK,
                           OGL.clear_override ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED, NULL);

        // Enable/disable fog
        SendDlgItemMessage(hWndDlg, IDC_FOG, BM_SETCHECK, OGL.fog ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED, NULL);
        SendDlgItemMessage(hWndDlg, IDC_MSAA, BM_SETCHECK, OGL.msaa == 4 ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED,
                           NULL);

        SendDlgItemMessage(hWndDlg, IDC_DITHEREDALPHATEST, BM_SETCHECK,
                           OGL.usePolygonStipple ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED, NULL);

        const auto cache_size = std::to_wstring(cache.maxBytes / 1048576);
        SendDlgItemMessage(hWndDlg, IDC_CACHEMEGS, WM_SETTEXT, NULL, (LPARAM)cache_size.c_str());

        SendMessage(hWndDlg, WM_COMMAND, MAKEWPARAM(IDC_TEXTUREFILTER, CBN_SELCHANGE),
                    (LPARAM)GetDlgItem(hWndDlg, IDC_TEXTUREFILTER));

        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            Config_ApplyDlgConfig(hWndDlg);
            EndDialog(hWndDlg, wParam);
            hConfigDlg = NULL;
            return TRUE;
        case IDCANCEL:
            EndDialog(hWndDlg, wParam);
            hConfigDlg = NULL;
            return TRUE;
        case IDC_WINDOWEDRES:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                const auto cb_hwnd = (HWND)lParam;
                const auto index = ComboBox_GetCurSel(cb_hwnd);

                const auto &preset = RESOLUTION_PRESETS[index];
                Edit_SetText(GetDlgItem(hWndDlg, IDC_WINDOWED_X), std::to_wstring(preset.width).c_str());
                Edit_SetText(GetDlgItem(hWndDlg, IDC_WINDOWED_Y), std::to_wstring(preset.height).c_str());
            }
            break;
        case IDC_WINDOWED_X:
        case IDC_WINDOWED_Y:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                std::wstring w_str(32, 0);
                std::wstring h_str(32, 0);
                Edit_GetText(GetDlgItem(hWndDlg, IDC_WINDOWED_X), w_str.data(), w_str.size());
                Edit_GetText(GetDlgItem(hWndDlg, IDC_WINDOWED_Y), h_str.data(), h_str.size());

                try
                {
                    OGL.windowedWidth = std::stoul(std::wstring(w_str));
                    OGL.windowedHeight = std::stoul(std::wstring(h_str));
                }
                catch (...)
                {
                    break;
                }

                const auto cb_hwnd = GetDlgItem(hWndDlg, IDC_WINDOWEDRES);
                select_current_resolution_in_combobox(cb_hwnd);
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

void Config_Show(HWND parent)
{
    DialogBox(g_tas_ctx.hinst, MAKEINTRESOURCE(IDD_CONFIGDLG), parent, (DLGPROC)ConfigDlgProc);
}
