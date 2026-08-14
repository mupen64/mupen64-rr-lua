/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <components/MovieDialog.hpp>
#include <Common.Views/Config.hpp>
#include <Common.Views/IDialogService.hpp>
#include <components/FilePicker.hpp>

struct t_movie_dialog_context
{
    MovieDialog::t_result user_result{};
    std::function<bool(const MovieDialog::t_result &)> on_confirm{};
    bool is_readonly{};
    HWND grid_hwnd{};
    bool is_closing{};
};

static t_movie_dialog_context g_ctx{};

static size_t count_button_presses(const std::vector<CoreButtons> &buttons, const int mask)
{
    size_t accumulator = 0;
    bool pressed = false;
    for (const auto btn : buttons)
    {
        const bool value = !!(btn.value >> mask & 1);

        if (value && !pressed)
        {
            accumulator++;
            pressed = true;
        }
        else if (!value)
        {
            pressed = false;
        }
    }
    return accumulator;
}

static size_t count_unused_inputs(const std::vector<CoreButtons> &buttons)
{
    size_t accumulator = 0;
    for (const auto btn : buttons)
    {
        if (btn.value == 0)
        {
            accumulator++;
        }
    }
    return accumulator;
}

static size_t count_joystick_frames(const std::vector<CoreButtons> &buttons)
{
    size_t accumulator = 0;
    for (const auto btn : buttons)
    {
        if (btn.y != 0 || btn.x != 0)
        {
            accumulator++;
        }
    }
    return accumulator;
}

static size_t count_input_changes(const std::vector<CoreButtons> &buttons)
{
    size_t accumulator = 0;
    CoreButtons last_input = {0};
    for (const auto btn : buttons)
    {
        if (btn.value != last_input.value)
        {
            accumulator++;
        }
        last_input = btn;
    }
    return accumulator;
}

static LRESULT CALLBACK dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    // List of dialog item IDs that shouldn't be interactable when in a specific mode
    const std::vector<int32_t> disabled_on_play = {IDC_RADIO_FROM_START, IDC_RADIO_FROM_ST, IDC_RADIO_FROM_EEPROM};
    const std::vector<int32_t> disabled_on_record = {};

    switch (msg)
    {
    case WM_INITDIALOG: {
        g_ctx.user_result.hwnd = hwnd;
        RECT grid_rect = get_window_rect_client_space(hwnd, GetDlgItem(hwnd, IDC_MOVIE_INFO_TEMPLATE));
        DestroyWindow(GetDlgItem(hwnd, IDC_MOVIE_INFO_TEMPLATE));

        g_ctx.grid_hwnd =
            CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL,
                           WS_TABSTOP | WS_VISIBLE | WS_CHILD | LVS_SINGLESEL | LVS_REPORT | LVS_SHOWSELALWAYS,
                           grid_rect.left, grid_rect.top, grid_rect.right - grid_rect.left,
                           grid_rect.bottom - grid_rect.top, hwnd, nullptr, g_main_ctx.hinst, NULL);

        ListView_SetExtendedListViewStyle(g_ctx.grid_hwnd,
                                          LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

        LVCOLUMN lv_column = {0};
        lv_column.mask = LVCF_FMT | LVCF_DEFAULTWIDTH | LVCF_TEXT | LVCF_SUBITEM;

        lv_column.pszText = const_cast<LPSTR>("Name");
        ListView_InsertColumn(g_ctx.grid_hwnd, 0, &lv_column);
        lv_column.pszText = const_cast<LPSTR>("Value");
        ListView_InsertColumn(g_ctx.grid_hwnd, 1, &lv_column);

        ListView_SetColumnWidth(g_ctx.grid_hwnd, 0, LVSCW_AUTOSIZE_USEHEADER);
        ListView_SetColumnWidth(g_ctx.grid_hwnd, 1, LVSCW_AUTOSIZE_USEHEADER);

        SetWindowText(hwnd, g_ctx.is_readonly ? "Play Movie" : "Record Movie");
        for (auto id : g_ctx.is_readonly ? disabled_on_play : disabled_on_record)
        {
            EnableWindow(GetDlgItem(hwnd, id), false);
        }
        SendMessage(GetDlgItem(hwnd, IDC_INI_DESCRIPTION), EM_SETLIMITTEXT, sizeof(core_vcr_movie_header::description),
                    0);
        SendMessage(GetDlgItem(hwnd, IDC_INI_AUTHOR), EM_SETLIMITTEXT, sizeof(core_vcr_movie_header::author), 0);

        SetDlgItemText(hwnd, IDC_INI_AUTHOR, g_config.last_movie_author.c_str());
        SetDlgItemText(hwnd, IDC_INI_DESCRIPTION, "");

        SetDlgItemText(hwnd, IDC_INI_MOVIEFILE, g_ctx.user_result.path.string().c_str());

        // workaround because initial selected button is "Start"
        SetFocus(GetDlgItem(hwnd, IDC_INI_AUTHOR));

        WinDarkMode::attach(hwnd);
        return FALSE;
    }
    case WM_DESTROY:
        DestroyWindow(g_ctx.grid_hwnd);
        break;

    case WM_CLOSE:
        g_ctx.user_result.path.clear();
        g_ctx.is_closing = true;
        EndDialog(hwnd, IDCANCEL);
        break;
    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDOK: {
            g_config.last_movie_type = g_ctx.user_result.start_flag;

            char author[sizeof(core_vcr_movie_header::author)] = {0};
            GetDlgItemText(hwnd, IDC_INI_AUTHOR, author, std::size(author));
            g_ctx.user_result.author = author;
            g_config.last_movie_author = g_ctx.user_result.author;

            char description[sizeof(core_vcr_movie_header::description)] = {0};
            GetDlgItemText(hwnd, IDC_INI_DESCRIPTION, description, std::size(description));
            g_ctx.user_result.description = description;

            const bool should_close = g_ctx.on_confirm(g_ctx.user_result);
            if (!should_close)
            {
                break;
            }

            EndDialog(hwnd, IDOK);
        }
        break;
        case IDCANCEL:
            g_ctx.user_result.path.clear();
            g_ctx.is_closing = true;
            EndDialog(hwnd, IDCANCEL);
            break;
        case IDC_INI_MOVIEFILE: {
            if (g_ctx.is_closing)
            {
                g_view_logger->warn("[MovieDialog] Tried to update movie file path while closing dialog");
                break;
            }

            char path[MAX_PATH] = {0};
            GetDlgItemText(hwnd, IDC_INI_MOVIEFILE, path, std::size(path));
            g_ctx.user_result.path = path;

            // User might not provide the m64 extension, so just force it to have that
            g_ctx.user_result.path.replace_extension(".m64");

            goto refresh;
        }
        case IDC_MOVIE_BROWSE: {
            std::filesystem::path path;
            if (g_ctx.is_readonly)
            {
                path = FilePicker::show_open_dialog("o_movie", hwnd, "*.m64;*.rec");
            }
            else
            {
                path = FilePicker::show_save_dialog("s_movie", hwnd, "*.m64;*.rec");
            }

            if (path.empty())
            {
                break;
            }

            SetDlgItemText(hwnd, IDC_INI_MOVIEFILE, path.string().c_str());
        }
        break;
        case IDC_RADIO_FROM_EEPROM:
            EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 1);
            EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 1);
            EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 1);
            g_ctx.user_result.start_flag = MOVIE_START_FROM_EEPROM;
            break;
        case IDC_RADIO_FROM_ST:
            EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 1);
            EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 1);
            EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 1);
            g_ctx.user_result.start_flag = MOVIE_START_FROM_SNAPSHOT;
            break;
        case IDC_RADIO_FROM_START:
            EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 1);
            EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 1);
            EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 1);
            g_ctx.user_result.start_flag = MOVIE_START_FROM_NOTHING;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return FALSE;

refresh:
    core_vcr_movie_header header = {};

    if (g_main_ctx.core_ctx->vcr_parse_header(g_ctx.user_result.path, &header) != Res_Ok)
    {
        return FALSE;
    }

    std::vector<CoreButtons> inputs = {};

    if (g_main_ctx.core_ctx->vcr_read_movie_inputs(g_ctx.user_result.path, inputs) != Res_Ok)
    {
        return FALSE;
    }

    std::vector<std::pair<std::string, std::string>> metadata;

    ListView_DeleteAllItems(g_ctx.grid_hwnd);

    metadata.emplace_back(
        std::make_pair("ROM", std::format("{} ({}, {})", (char *)header.rom_name,
                                          g_main_ctx.core_ctx->vr_country_code_to_country_name(header.rom_country),
                                          std::format("{:#08x}", header.rom_crc1))));

    metadata.emplace_back(
        std::make_pair("Length", std::format("{} ({} input)", header.length_vis, header.length_samples)));
    metadata.emplace_back(
        std::make_pair("Duration", format_duration((double)header.length_vis / (double)header.vis_per_second)));
    metadata.emplace_back(std::make_pair(
        "Rerecords",
        std::to_string(static_cast<uint64_t>(header.extended_data.rerecord_count) << 32 | header.rerecord_count)));

    metadata.emplace_back(std::make_pair("Video Plugin", header.video_plugin_name));
    metadata.emplace_back(std::make_pair("Input Plugin", header.input_plugin_name));
    metadata.emplace_back(std::make_pair("Sound Plugin", header.audio_plugin_name));
    metadata.emplace_back(std::make_pair("RSP Plugin", header.rsp_plugin_name));

    for (int i = 0; i < 4; ++i)
    {
        std::string desc;

        desc += header.controller_flags & CONTROLLER_X_PRESENT(i) ? "Present" : "Disconnected";

        if (header.controller_flags & CONTROLLER_X_MEMPAK(i)) desc += " with mempak";
        if (header.controller_flags & CONTROLLER_X_RUMBLE(i)) desc += " with rumble";

        metadata.emplace_back(std::make_pair(std::format("Controller {}", i + 1), desc));
    }

    metadata.emplace_back(std::make_pair(
        "WiiVC", header.extended_version == 0 ? "Unknown" : (header.extended_flags.wii_vc ? "Enabled" : "Disabled")));

    metadata.emplace_back(std::make_pair(
        "Accurate RDP completion", header.extended_version < 3
                                       ? "Unknown"
                                       : (header.extended_flags.accurate_rdp_completion ? "Enabled" : "Disabled")));

    metadata.emplace_back(std::make_pair("CPU counter factor", header.extended_version < 4
                                                                   ? std::string("Unknown")
                                                                   : std::format("{}", header.extended_data.cpu_cf)));

    metadata.emplace_back(std::make_pair(
        "RCP lag factor",
        header.extended_version < 4 ? std::string("Unknown") : std::format("{}", header.extended_data.rcp_lag_factor)));

    char authorship[5] = {0};
    memcpy(authorship, header.extended_data.authorship_tag, sizeof(header.extended_data.authorship_tag));

    metadata.emplace_back(std::make_pair("Authorship", header.extended_version == 0 ? "Unknown" : authorship));

    metadata.emplace_back(std::make_pair("A Presses", std::to_string(count_button_presses(inputs, 7))));
    metadata.emplace_back(std::make_pair("B Presses", std::to_string(count_button_presses(inputs, 6))));
    metadata.emplace_back(std::make_pair("Z Presses", std::to_string(count_button_presses(inputs, 5))));
    metadata.emplace_back(std::make_pair("S Presses", std::to_string(count_button_presses(inputs, 4))));
    metadata.emplace_back(std::make_pair("R Presses", std::to_string(count_button_presses(inputs, 12))));

    metadata.emplace_back(std::make_pair("C^ Presses", std::to_string(count_button_presses(inputs, 11))));
    metadata.emplace_back(std::make_pair("Cv Presses", std::to_string(count_button_presses(inputs, 10))));
    metadata.emplace_back(std::make_pair("C< Presses", std::to_string(count_button_presses(inputs, 9))));
    metadata.emplace_back(std::make_pair("C> Presses", std::to_string(count_button_presses(inputs, 8))));

    const auto lag_frames = std::max((int64_t)0, (int64_t)header.length_vis - 2 * (int64_t)header.length_samples);
    metadata.emplace_back(std::make_pair("Lag Frames (approximation)", std::to_string(lag_frames)));
    metadata.emplace_back(std::make_pair("Unused Inputs", std::to_string(count_unused_inputs(inputs))));
    metadata.emplace_back(std::make_pair("Joystick Frames", std::to_string(count_joystick_frames(inputs))));
    metadata.emplace_back(std::make_pair("Input Changes", std::to_string(count_input_changes(inputs))));

    SetDlgItemText(hwnd, IDC_INI_AUTHOR, header.author);
    SetDlgItemText(hwnd, IDC_INI_DESCRIPTION, header.description);

    CheckDlgButton(hwnd, IDC_RADIO_FROM_ST, header.startFlags == MOVIE_START_FROM_SNAPSHOT);
    CheckDlgButton(hwnd, IDC_RADIO_FROM_START, header.startFlags == MOVIE_START_FROM_NOTHING);
    CheckDlgButton(hwnd, IDC_RADIO_FROM_EEPROM, header.startFlags == MOVIE_START_FROM_EEPROM);

    LVITEM lv_item = {0};
    lv_item.mask = LVIF_TEXT | LVIF_DI_SETITEM;
    for (int i = 0; i < metadata.size(); ++i)
    {
        lv_item.iItem = i;

        lv_item.iSubItem = 0;
        lv_item.pszText = (LPTSTR)metadata[i].first.c_str();
        ListView_InsertItem(g_ctx.grid_hwnd, &lv_item);

        lv_item.iSubItem = 1;
        lv_item.pszText = (LPTSTR)metadata[i].second.c_str();
        ListView_SetItem(g_ctx.grid_hwnd, &lv_item);
    }

    ListView_SetColumnWidth(g_ctx.grid_hwnd, 0, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth(g_ctx.grid_hwnd, 1, LVSCW_AUTOSIZE_USEHEADER);

    return FALSE;
}

static std::filesystem::path get_default_movie_path(bool readonly)
{
    const auto rom_hdr = g_main_ctx.core_ctx->vr_get_rom_header();

    if (g_config.recent_movie_paths.empty() || !readonly)
    {
        char rom_name[sizeof(rom_hdr->nom) + 1]{};
        std::memcpy(rom_name, rom_hdr->nom, sizeof(rom_hdr->nom));
        const auto rom_country = g_main_ctx.core_ctx->vr_country_code_to_country_name(rom_hdr->Country_code);
        return std::format("{} ({}).m64", rom_name, rom_country);
    }

    return g_config.recent_movie_paths[0];
}

MovieDialog::t_result MovieDialog::show(bool readonly, const std::function<bool(const t_result &)> &on_confirm)
{
    g_ctx.is_readonly = readonly;
    g_ctx.on_confirm = on_confirm;
    g_ctx.user_result.path = get_default_movie_path(readonly);
    g_ctx.user_result.start_flag = g_config.last_movie_type;
    g_ctx.user_result.author = g_config.last_movie_author;
    g_ctx.user_result.description = "";
    g_ctx.is_closing = false;

    DialogBox(g_main_ctx.hinst, MAKEINTRESOURCE(IDD_MOVIE_DIALOG), g_main_ctx.hwnd, (DLGPROC)dlgproc);

    return g_ctx.user_result;
}
