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

struct
{
    WORD width{};
    WORD height{};
    std::wstring description{};
} windowedModes[numWindowedModes] = {
    {320, 240, L"320 x 240"},     {400, 300, L"400 x 300"},     {480, 360, L"480 x 360"},
    {640, 480, L"640 x 480"},     {800, 600, L"800 x 600"},     {960, 720, L"960 x 720"},
    {1024, 768, L"1024 x 768"},   {1152, 864, L"1152 x 864"},   {1280, 960, L"1280 x 960"},
    {1280, 1024, L"1280 x 1024"}, {1440, 1080, L"1440 x 1080"}, {1600, 1200, L"1600 x 1200"}};

static std::filesystem::path get_config_path()
{
    return g_tas_ctx.config_directory / CONFIG_FILE_NAME;
}

void EnableCustom(HWND hWndDlg, BOOL enable)
{
    EnableWindow(GetDlgItem(hWndDlg, IDC_WINDOWED_X), enable);
    EnableWindow(GetDlgItem(hWndDlg, IDC_WINDOWED_Y), enable);
}

static void Config_SetDefaults()
{
    OGL.fog = TRUE;
    OGL.msaa = 0;
    OGL.windowedWidth = 640;
    OGL.windowedHeight = 480;
    OGL.forceBilinear = FALSE;
    cache.maxBytes = 32 * 1048576;
    OGL.textureFilter = TextureFilter::None;
    OGL.usePolygonStipple = FALSE;
}

void Config_LoadConfig()
{
    auto json_path = get_config_path();

    if (!std::filesystem::exists(json_path)) {
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
        OGL.forceBilinear = j["force_bilinear"];
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
        g_ef->log_warn(L"Config load failed, using defaults...");
        Config_SetDefaults();
    }
}

void Config_SaveConfig()
{
    json j = json::object({
        {"windowed_width", OGL.windowedWidth},
        {"windowed_height", OGL.windowedHeight},
        {"force_bilinear", (bool)OGL.forceBilinear},
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

    OGL.forceBilinear = (SendDlgItemMessage(hWndDlg, IDC_FORCEBILINEAR, BM_GETCHECK, NULL, NULL) == BST_CHECKED);
    OGL.textureFilter = (TextureFilter)SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_GETCURSEL, NULL, NULL);
    OGL.filterScale = SendDlgItemMessage(hWndDlg, IDC_FSCALE, TBM_GETPOS, NULL, NULL);
    OGL.fog = (SendDlgItemMessage(hWndDlg, IDC_FOG, BM_GETCHECK, NULL, NULL) == BST_CHECKED);
    OGL.msaa = (SendDlgItemMessage(hWndDlg, IDC_MSAA, BM_GETCHECK, NULL, NULL) == BST_CHECKED) ? 4 : 0;
    OGL.originAdjust = (OGL.textureFilter == TextureFilter::SaI ? 0.25 : 0.50);
    OGL.ignoreScissor = (SendDlgItemMessage(hWndDlg, IDC_SCISSOR, BM_GETCHECK, NULL, NULL) == BST_CHECKED);
    OGL.clear_override = (SendDlgItemMessage(hWndDlg, IDC_CLEAR, BM_GETCHECK, NULL, NULL) == BST_CHECKED);

    i = SendDlgItemMessage(hWndDlg, IDC_WINDOWEDRES, CB_GETCURSEL, 0, 0);
    if (i == SendMessage(GetDlgItem(hWndDlg, IDC_WINDOWEDRES), CB_GETCOUNT, 0, 0) - 1)
    {
        wchar_t val[32]{};
        SendMessage(GetDlgItem(hWndDlg, IDC_WINDOWED_X), WM_GETTEXT, std::size(val), (LPARAM)val);
        OGL.windowedWidth = _wtoi(val);
        SendMessage(GetDlgItem(hWndDlg, IDC_WINDOWED_Y), WM_GETTEXT, std::size(val), (LPARAM)val);
        OGL.windowedHeight = _wtoi(val);
    }
    else
    {
        OGL.windowedWidth = windowedModes[i].width;
        OGL.windowedHeight = windowedModes[i].height;
    }

    OGL.usePolygonStipple =
        (SendDlgItemMessage(hWndDlg, IDC_DITHEREDALPHATEST, BM_GETCHECK, NULL, NULL) == BST_CHECKED);

    Config_SaveConfig();
    Config_LoadConfig();

    const auto needs_restart =
        OGL.forceBilinear != prev_OGL.forceBilinear || OGL.textureFilter != prev_OGL.textureFilter ||
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

BOOL CALLBACK ConfigDlgProc(HWND hWndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    bool custom = true;

    switch (message)
    {
    case WM_INITDIALOG: {
        hConfigDlg = hWndDlg;

        // Fill windowed mode resolution
        for (int i = 0; i < numWindowedModes; i++)
        {
            ComboBox_AddString(GetDlgItem(hWndDlg, IDC_WINDOWEDRES), windowedModes[i].description.c_str());
            if ((OGL.windowedWidth == windowedModes[i].width) && (OGL.windowedHeight == windowedModes[i].height))
            {
                SendDlgItemMessage(hWndDlg, IDC_WINDOWEDRES, CB_SETCURSEL, i, 0);
                custom = false;
            }
        }

        SendDlgItemMessage(hWndDlg, IDC_WINDOWEDRES, CB_ADDSTRING, 0, (LPARAM)L"Custom...");

        SendDlgItemMessage(hWndDlg, IDC_WINDOWED_X, WM_SETTEXT, 0, (LPARAM)std::to_wstring(OGL.windowedWidth).c_str());
        SendDlgItemMessage(hWndDlg, IDC_WINDOWED_Y, WM_SETTEXT, 0, (LPARAM)std::to_wstring(OGL.windowedHeight).c_str());

        if (custom)
        {
            int num = SendDlgItemMessage(hWndDlg, IDC_WINDOWEDRES, CB_GETCOUNT, 0, 0) - 1;
            EnableCustom(hWndDlg, TRUE);
            SendDlgItemMessage(hWndDlg, IDC_WINDOWEDRES, CB_SETCURSEL, num, 0);
        }

        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_ADDSTRING, 0, (LPARAM)L"None");
        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_ADDSTRING, 0, (LPARAM)L"2xSaI");
        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_ADDSTRING, 0, (LPARAM)L"xBRZ");
        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_ADDSTRING, 0, (LPARAM)L"Hqx");
        SendDlgItemMessage(hWndDlg, IDC_TEXTUREFILTER, CB_SETCURSEL, (int)OGL.textureFilter, 0);
        SendMessage(GetDlgItem(hWndDlg, IDC_FSCALE), TBM_SETPOS, TRUE, OGL.filterScale);

        SendDlgItemMessage(hWndDlg, IDC_FORCEBILINEAR, BM_SETCHECK,
                           OGL.forceBilinear ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED, NULL);
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
                if (SendDlgItemMessage(hWndDlg, IDC_WINDOWEDRES, CB_GETCURSEL, 0, 0) ==
                    SendDlgItemMessage(hWndDlg, IDC_WINDOWEDRES, CB_GETCOUNT, 0, 0) - 1)
                {
                    EnableCustom(hWndDlg, TRUE);
                }
                else
                {
                    EnableCustom(hWndDlg, FALSE);
                }
            }
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
