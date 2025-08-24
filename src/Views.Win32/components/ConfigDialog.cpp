/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <ActionManager.h>
#include <Config.h>
#include <DialogService.h>
#include <Messenger.h>
#include <Plugin.h>
#include <capture/EncodingManager.h>
#include <components/FilePicker.h>
#include <components/SettingsListView.h>
#include <components/configdialog.h>
#include <lua/LuaManager.h>

#include <algorithm>

#define WM_EDIT_END (WM_USER + 19)
#define WM_PLUGIN_DISCOVERY_FINISHED (WM_USER + 22)

/**
 * Represents a group of options in the settings.
 */
struct t_options_group {
    /**
     * The group's unique identifier.
     */
    size_t id;

    /**
     * The group's name.
     */
    std::wstring name;
};

/**
 * Represents a settings option.
 */
struct t_options_item {
    enum class Type {
        Bool,
        Number,
        Enum,
        String,
        Hotkey,
    };

    typedef std::variant<int32_t, std::wstring, Hotkey::t_hotkey> data_variant;

    struct t_readonly_property {
        std::function<data_variant()> get{};

        explicit t_readonly_property(const std::function<data_variant()>& get)
        {
            this->get = get;
        }
    };


    struct t_readwrite_property : t_readonly_property {
        std::function<void(const data_variant&)> set{};

        t_readwrite_property(const std::function<data_variant()>& get, const std::function<void(const data_variant&)>& set) :
            t_readonly_property(get)
        {
            this->set = set;
        }
    };


    /**
     * The option's backing data type.
     */
    Type type;

    /**
     * The group this option belongs to.
     */
    size_t group_id;

    /**
     * The option's display name.
     */
    std::wstring name{};

    /**
     * The option's tooltip, or an empty string if no tooltip is set.
     */
    std::wstring tooltip{};

    t_readwrite_property current_value;

    t_readonly_property initial_value = t_readonly_property([] -> data_variant {
        runtime_assert(false, L"Initial value not set for option");
        return data_variant{};
    });

    t_readonly_property default_value;

    std::vector<std::pair<std::wstring, int32_t>> possible_values = {};

    /**
     * Function which returns whether the option can be changed. Useful for values which shouldn't be changed during emulation.
     */
    std::function<bool()> is_readonly = [] {
        return false;
    };

    /**
     * Gets the name of the option item.
     */
    [[nodiscard]] std::wstring get_name() const;

    /**
     * Gets the value name for the current backing data, or a fallback name if no match is found.
     */
    [[nodiscard]] std::wstring get_value_name() const;

    /**
     * Resets the value of the option to the default value.
     */
    void reset_to_default() const;

    /**
     * \brief Gets neatly formatted information about the option.
     */
    std::wstring get_friendly_info() const;
};

t_plugin_discovery_result plugin_discovery_result;
std::vector<t_options_group> g_option_groups;
std::vector<t_options_item> g_option_items;
HWND g_lv_hwnd;
HWND g_edit_hwnd;
size_t g_edit_option_item_index;
t_config g_prev_config;

std::thread g_plugin_discovery_thread;

// Whether a plugin rescan is needed. Set when modifying the plugin path.
bool g_plugin_discovery_rescan = false;

std::wstring t_options_item::get_name() const
{
    if (type == Type::Hotkey)
    {
        return ActionManager::get_display_name(name, true);
    }
    return name;
}

std::wstring t_options_item::get_value_name() const
{
    const auto value = current_value.get();

    switch (type)
    {
    case Type::Bool:
        return std::get<int32_t>(value) != 0 ? L"On" : L"Off";
    case Type::Number:
        return std::to_wstring(std::get<int32_t>(value));
    case Type::Enum:
        {
            const auto enum_value = std::get<int32_t>(value);

            for (const auto& pair : possible_values)
            {
                if (enum_value == pair.second)
                {
                    return pair.first;
                }
            }

            return std::format(L"Unknown ({})", enum_value);
        }
    case Type::String:
        return std::get<std::wstring>(value);
    case Type::Hotkey:
        return std::get<Hotkey::t_hotkey>(value).to_wstring();
    default:
        runtime_assert(false, L"Unhandled option type in t_options_item::get_value_name");
    }
    return L"";
}

void t_options_item::reset_to_default() const
{
    current_value.set(default_value.get());
}

std::wstring t_options_item::get_friendly_info() const
{
    std::wstring str = tooltip.empty() ? L"(no further information available)" : tooltip;

    if (possible_values.empty())
    {
        return str;
    }

    str += L"\r\n\r\n";
    for (const auto& pair : possible_values)
    {
        str += std::format(L"{} - {}", pair.second, pair.first);

        if (pair.second == std::get<int32_t>(current_value.get()))
        {
            str += L" (default)";
        }

        str += L"\r\n";
    }

    return str;
}

INT_PTR CALLBACK plugin_discovery_dlgproc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    static HWND g_pldlv_hwnd;

    switch (msg)
    {
    case WM_INITDIALOG:
        {
            RECT rect{};
            GetClientRect(hwnd, &rect);

            g_pldlv_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL, WS_TABSTOP | WS_VISIBLE | WS_CHILD | LVS_SINGLESEL | LVS_REPORT | LVS_SHOWSELALWAYS, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hwnd, nullptr, g_app_instance, NULL);

            ListView_SetExtendedListViewStyle(g_pldlv_hwnd,
                                              LVS_EX_GRIDLINES |
                                              LVS_EX_FULLROWSELECT |
                                              LVS_EX_DOUBLEBUFFER);

            LVCOLUMN lv_column = {0};
            lv_column.mask = LVCF_FMT | LVCF_DEFAULTWIDTH | LVCF_TEXT |
            LVCF_SUBITEM;

            lv_column.pszText = const_cast<LPWSTR>(L"Plugin");
            ListView_InsertColumn(g_pldlv_hwnd, 0, &lv_column);
            lv_column.pszText = const_cast<LPWSTR>(L"Error");
            ListView_InsertColumn(g_pldlv_hwnd, 1, &lv_column);

            LV_ITEM lv_item = {0};
            lv_item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
            lv_item.pszText = LPSTR_TEXTCALLBACK;

            size_t i = 0;
            for (const auto& pair : plugin_discovery_result.results)
            {
                if (!pair.second.empty())
                {
                    lv_item.lParam = (int)i;
                    lv_item.iItem = (int)i;
                    ListView_InsertItem(g_pldlv_hwnd, &lv_item);
                }

                i++;
            }

            ListView_SetColumnWidth(g_pldlv_hwnd, 0, LVSCW_AUTOSIZE_USEHEADER);
            ListView_SetColumnWidth(g_pldlv_hwnd, 1, LVSCW_AUTOSIZE_USEHEADER);

            return TRUE;
        }
    case WM_NOTIFY:
        {
            switch (((LPNMHDR)l_param)->code)
            {
            case LVN_GETDISPINFO:
                {
                    auto plvdi = reinterpret_cast<NMLVDISPINFO*>(l_param);
                    const auto pair = plugin_discovery_result.results[plvdi->item.lParam];
                    switch (plvdi->item.iSubItem)
                    {
                    case 0:
                        StrNCpy(plvdi->item.pszText, pair.first.filename().c_str(), plvdi->item.cchTextMax);
                        break;
                    case 1:
                        {
                            StrNCpy(plvdi->item.pszText, pair.second.c_str(), plvdi->item.cchTextMax);
                            break;
                        }
                    default:
                        break;
                    }
                }
            default:
                break;
            }


            break;
        }
    case WM_DESTROY:
        EndDialog(hwnd, LOWORD(w_param));
        return TRUE;
    case WM_CLOSE:
        EndDialog(hwnd, IDOK);
        break;
    case WM_COMMAND:
        switch (LOWORD(w_param))
        {
        default:
            break;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

void build_rom_browser_path_list(const HWND dialog_hwnd)
{
    const HWND hwnd = GetDlgItem(dialog_hwnd, IDC_ROMBROWSER_DIR_LIST);

    SendMessage(hwnd, LB_RESETCONTENT, 0, 0);

    for (const std::wstring& str : g_config.rombrowser_rom_paths)
    {
        SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)str.c_str());
    }
}

INT_PTR CALLBACK directories_cfg(const HWND hwnd, const UINT message, const WPARAM w_param, LPARAM l_param)
{
    const auto lpnmhdr = reinterpret_cast<LPNMHDR>(l_param);
    wchar_t path[MAX_PATH] = {0};

    switch (message)
    {
    case WM_INITDIALOG:
        build_rom_browser_path_list(hwnd);

        SendMessage(GetDlgItem(hwnd, IDC_RECURSION), BM_SETCHECK, g_config.is_rombrowser_recursion_enabled ? BST_CHECKED : BST_UNCHECKED, 0);

        if (g_config.is_default_plugins_directory_used)
        {
            SendMessage(GetDlgItem(hwnd, IDC_DEFAULT_PLUGINS_CHECK), BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwnd, IDC_PLUGINS_DIR), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_PLUGINS_DIR), FALSE);
        }
        if (g_config.is_default_saves_directory_used)
        {
            SendMessage(GetDlgItem(hwnd, IDC_DEFAULT_SAVES_CHECK), BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwnd, IDC_SAVES_DIR), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_SAVES_DIR), FALSE);
        }
        if (g_config.is_default_screenshots_directory_used)
        {
            SendMessage(GetDlgItem(hwnd, IDC_DEFAULT_SCREENSHOTS_CHECK), BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwnd, IDC_SCREENSHOTS_DIR), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_SCREENSHOTS_DIR), FALSE);
        }
        if (g_config.is_default_backups_directory_used)
        {
            SendMessage(GetDlgItem(hwnd, IDC_DEFAULT_BACKUPS_CHECK), BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwnd, IDC_BACKUPS_DIR), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_BACKUPS_DIR), FALSE);
        }

        SetDlgItemText(hwnd, IDC_PLUGINS_DIR, g_config.plugins_directory.c_str());
        SetDlgItemText(hwnd, IDC_SAVES_DIR, g_config.saves_directory.c_str());
        SetDlgItemText(hwnd, IDC_SCREENSHOTS_DIR, g_config.screenshots_directory.c_str());
        SetDlgItemText(hwnd, IDC_BACKUPS_DIR, g_config.backups_directory.c_str());

        if (g_core_ctx->vr_get_launched())
        {
            EnableWindow(GetDlgItem(hwnd, IDC_DEFAULT_SAVES_CHECK), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_SAVES_DIR), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_SAVES_DIR), FALSE);
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(w_param))
        {
        case IDC_PLUGINS_DIR:
            {
                const auto prev_plugins_dir = g_config.plugins_directory;

                GetDlgItemText(hwnd, IDC_PLUGINS_DIR, path, std::size(path));
                g_config.plugins_directory = path;

                if (g_config.plugins_directory != prev_plugins_dir)
                {
                    g_plugin_discovery_rescan = true;
                }
                break;
            }
        case IDC_SAVES_DIR:
            GetDlgItemText(hwnd, IDC_SAVES_DIR, path, std::size(path));
            g_config.saves_directory = path;
            break;
        case IDC_SCREENSHOTS_DIR:
            GetDlgItemText(hwnd, IDC_SCREENSHOTS_DIR, path, std::size(path));
            g_config.screenshots_directory = path;
            break;
        case IDC_BACKUPS_DIR:
            GetDlgItemText(hwnd, IDC_BACKUPS_DIR, path, std::size(path));
            g_config.backups_directory = path;
            break;
        case IDC_RECURSION:
            g_config.is_rombrowser_recursion_enabled = IsDlgButtonChecked(hwnd, IDC_RECURSION) == BST_CHECKED;
            break;
        case IDC_ADD_BROWSER_DIR:
            {
                const auto path = FilePicker::show_folder_dialog(L"f_roms", hwnd);
                if (path.empty())
                {
                    break;
                }
                g_config.rombrowser_rom_paths.push_back(path);
                build_rom_browser_path_list(hwnd);
                break;
            }
        case IDC_REMOVE_BROWSER_DIR:
            {
                const int32_t selected_index = ListBox_GetCurSel(GetDlgItem(hwnd, IDC_ROMBROWSER_DIR_LIST));
                if (selected_index == -1)
                {
                    break;
                }
                g_config.rombrowser_rom_paths.erase(g_config.rombrowser_rom_paths.begin() + selected_index);
                build_rom_browser_path_list(hwnd);
                break;
            }
        case IDC_REMOVE_BROWSER_ALL:
            g_config.rombrowser_rom_paths.clear();
            build_rom_browser_path_list(hwnd);
            break;
        case IDC_DEFAULT_PLUGINS_CHECK:
            {
                const auto prev = g_config.is_default_plugins_directory_used;
                g_config.is_default_plugins_directory_used = IsDlgButtonChecked(hwnd, IDC_DEFAULT_PLUGINS_CHECK) == BST_CHECKED;
                EnableWindow(GetDlgItem(hwnd, IDC_PLUGINS_DIR), !g_config.is_default_plugins_directory_used);
                EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_PLUGINS_DIR), !g_config.is_default_plugins_directory_used);
                if (g_config.is_default_plugins_directory_used != prev)
                {
                    g_plugin_discovery_rescan = true;
                }
            }
            break;
        case IDC_DEFAULT_BACKUPS_CHECK:
            {
                g_config.is_default_backups_directory_used = IsDlgButtonChecked(hwnd, IDC_DEFAULT_BACKUPS_CHECK) == BST_CHECKED;
                EnableWindow(GetDlgItem(hwnd, IDC_BACKUPS_DIR), !g_config.is_default_backups_directory_used);
                EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_BACKUPS_DIR), !g_config.is_default_backups_directory_used);
            }
            break;
        case IDC_PLUGIN_DIRECTORY_HELP:
            {
                MessageBox(hwnd, L"Changing the plugin directory may introduce bugs to some plugins.", L"Info", MB_ICONINFORMATION | MB_OK);
            }
            break;
        case IDC_CHOOSE_PLUGINS_DIR:
            {
                const auto path = FilePicker::show_folder_dialog(L"f_plugins", hwnd);
                if (path.empty())
                {
                    break;
                }
                g_config.plugins_directory = path;
                SetDlgItemText(hwnd, IDC_PLUGINS_DIR, g_config.plugins_directory.c_str());
            }
            break;
        case IDC_DEFAULT_SAVES_CHECK:
            {
                g_config.is_default_saves_directory_used = IsDlgButtonChecked(hwnd, IDC_DEFAULT_SAVES_CHECK) == BST_CHECKED;
                EnableWindow(GetDlgItem(hwnd, IDC_SAVES_DIR), !g_config.is_default_saves_directory_used);
                EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_SAVES_DIR), !g_config.is_default_saves_directory_used);
            }
            break;
        case IDC_CHOOSE_SAVES_DIR:
            {
                const auto path = FilePicker::show_folder_dialog(L"f_saves", hwnd);
                if (path.empty())
                {
                    break;
                }
                g_config.saves_directory = path;
                SetDlgItemText(hwnd, IDC_SAVES_DIR, g_config.saves_directory.c_str());
            }
            break;
        case IDC_DEFAULT_SCREENSHOTS_CHECK:
            {
                g_config.is_default_screenshots_directory_used = IsDlgButtonChecked(hwnd, IDC_DEFAULT_SCREENSHOTS_CHECK) == BST_CHECKED;
                EnableWindow(GetDlgItem(hwnd, IDC_SCREENSHOTS_DIR), !g_config.is_default_screenshots_directory_used);
                EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_SCREENSHOTS_DIR), !g_config.is_default_screenshots_directory_used);
            }
            break;
        case IDC_CHOOSE_SCREENSHOTS_DIR:
            {
                const auto path = FilePicker::show_folder_dialog(L"f_screenshots", hwnd);
                if (path.empty())
                {
                    break;
                }
                g_config.screenshots_directory = path;
                SetDlgItemText(hwnd, IDC_SCREENSHOTS_DIR, g_config.screenshots_directory.c_str());
            }
            break;
        case IDC_CHOOSE_BACKUPS_DIR:
            {
                const auto path = FilePicker::show_folder_dialog(L"f_backups", hwnd);
                if (path.empty())
                {
                    break;
                }
                g_config.backups_directory = path;
                SetDlgItemText(hwnd, IDC_BACKUPS_DIR, g_config.backups_directory.c_str());
            }
            break;
        default:
            break;
        }
        break;
    case WM_NOTIFY:
        if (lpnmhdr->code == PSN_SETACTIVE)
        {
            g_config.settings_tab = 1;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

void update_plugin_selection(const HWND hwnd, const int32_t id, const std::filesystem::path& path)
{
    for (int i = 0; i < SendDlgItemMessage(hwnd, id, CB_GETCOUNT, 0, 0); ++i)
    {
        if (const auto plugin = (Plugin*)SendDlgItemMessage(hwnd, id, CB_GETITEMDATA, i, 0); plugin->path() == path)
        {
            ComboBox_SetCurSel(GetDlgItem(hwnd, id), i);
            break;
        }
    }
    SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(id, 0), 0);
}

Plugin* get_selected_plugin(const HWND hwnd, const int id)
{
    const int i = SendDlgItemMessage(hwnd, id, CB_GETCURSEL, 0, 0);
    const auto res = SendDlgItemMessage(hwnd, id, CB_GETITEMDATA, i, 0);
    return res == CB_ERR ? nullptr : (Plugin*)res;
}

static void start_plugin_discovery(const HWND hwnd)
{
    g_view_logger->trace("[ConfigDialog] start_plugin_discovery");
    plugin_discovery_result = PluginUtil::discover_plugins(Config::plugin_directory());

    PostMessage(hwnd, WM_PLUGIN_DISCOVERY_FINISHED, 0, 0);
}

static void refresh_plugins_page(const HWND hwnd)
{
    g_view_logger->trace("[ConfigDialog] refresh_plugins_page");

    plugin_discovery_result = {};

    if (g_config.plugin_discovery_async)
    {
        SetDlgItemText(hwnd, IDC_PLUGIN_WARNING, L"Discovering plugins...");

        if (g_plugin_discovery_thread.joinable())
        {
            g_plugin_discovery_thread.join();
        }

        g_plugin_discovery_thread = std::thread([=] {
            start_plugin_discovery(hwnd);
        });
    }
    else
    {
        start_plugin_discovery(hwnd);
    }
}

static void update_plugin_buttons_enabled_state(HWND hwnd)
{
    auto combobox_has_selection = [](HWND hwnd) {
        return ComboBox_GetItemData(hwnd, ComboBox_GetCurSel(hwnd)) && ComboBox_GetCurSel(hwnd) != CB_ERR;
    };

    const auto has_video_plugin_selection = combobox_has_selection(GetDlgItem(hwnd, IDC_COMBO_GFX));
    const auto has_audio_plugin_selection = combobox_has_selection(GetDlgItem(hwnd, IDC_COMBO_SOUND));
    const auto has_input_plugin_selection = combobox_has_selection(GetDlgItem(hwnd, IDC_COMBO_INPUT));
    const auto has_rsp_plugin_selection = combobox_has_selection(GetDlgItem(hwnd, IDC_COMBO_RSP));

    EnableWindow(GetDlgItem(hwnd, IDM_VIDEO_SETTINGS), has_video_plugin_selection);
    EnableWindow(GetDlgItem(hwnd, IDGFXTEST), has_video_plugin_selection);
    EnableWindow(GetDlgItem(hwnd, IDGFXABOUT), has_video_plugin_selection);

    EnableWindow(GetDlgItem(hwnd, IDM_AUDIO_SETTINGS), has_audio_plugin_selection);
    EnableWindow(GetDlgItem(hwnd, IDSOUNDTEST), has_audio_plugin_selection);
    EnableWindow(GetDlgItem(hwnd, IDSOUNDABOUT), has_audio_plugin_selection);

    EnableWindow(GetDlgItem(hwnd, IDM_INPUT_SETTINGS), has_input_plugin_selection);
    EnableWindow(GetDlgItem(hwnd, IDINPUTTEST), has_input_plugin_selection);
    EnableWindow(GetDlgItem(hwnd, IDINPUTABOUT), has_input_plugin_selection);

    EnableWindow(GetDlgItem(hwnd, IDM_RSP_SETTINGS), has_rsp_plugin_selection);
    EnableWindow(GetDlgItem(hwnd, IDRSPTEST), has_rsp_plugin_selection);
    EnableWindow(GetDlgItem(hwnd, IDRSPABOUT), has_rsp_plugin_selection);
}

INT_PTR CALLBACK plugins_cfg(const HWND hwnd, const UINT message, const WPARAM w_param, const LPARAM l_param)
{
    const auto lpnmhdr = reinterpret_cast<LPNMHDR>(l_param);

    [[maybe_unused]] char path_buffer[_MAX_PATH];

    switch (message)
    {
    case WM_CLOSE:
        EndDialog(hwnd, IDOK);
        break;
    case WM_DESTROY:
        if (g_plugin_discovery_thread.joinable())
        {
            g_plugin_discovery_thread.join();
        }
        break;
    case WM_INITDIALOG:
        {
            SendDlgItemMessage(hwnd, IDB_DISPLAY, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)LoadImage(g_app_instance, MAKEINTRESOURCE(IDB_DISPLAY), IMAGE_BITMAP, 0, 0, 0));
            SendDlgItemMessage(hwnd, IDB_CONTROL, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)LoadImage(g_app_instance, MAKEINTRESOURCE(IDB_CONTROL), IMAGE_BITMAP, 0, 0, 0));
            SendDlgItemMessage(hwnd, IDB_SOUND, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)LoadImage(g_app_instance, MAKEINTRESOURCE(IDB_SOUND), IMAGE_BITMAP, 0, 0, 0));
            SendDlgItemMessage(hwnd, IDB_RSP, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)LoadImage(g_app_instance, MAKEINTRESOURCE(IDB_RSP), IMAGE_BITMAP, 0, 0, 0));

            refresh_plugins_page(hwnd);

            return TRUE;
        }
    case WM_PLUGIN_DISCOVERY_FINISHED:
        {
            std::vector<std::pair<std::filesystem::path, std::wstring>> broken_plugins;

            std::ranges::copy_if(plugin_discovery_result.results, std::back_inserter(broken_plugins), [](const auto& pair) {
                return !pair.second.empty();
            });

            if (broken_plugins.empty())
            {
                SetDlgItemText(hwnd, IDC_PLUGIN_WARNING, L"");
            }
            else
            {
                SetDlgItemText(hwnd, IDC_PLUGIN_WARNING, std::format(L"Not all discovered plugins shown. {} plugin(s) failed to load.", broken_plugins.size()).c_str());
            }

            EnableWindow(GetDlgItem(hwnd, IDC_PLUGIN_DISCOVERY_INFO), !broken_plugins.empty());

            ComboBox_ResetContent(GetDlgItem(hwnd, IDC_COMBO_GFX));
            ComboBox_ResetContent(GetDlgItem(hwnd, IDC_COMBO_SOUND));
            ComboBox_ResetContent(GetDlgItem(hwnd, IDC_COMBO_INPUT));
            ComboBox_ResetContent(GetDlgItem(hwnd, IDC_COMBO_RSP));

            for (const auto& plugin : plugin_discovery_result.plugins)
            {
                int32_t id = 0;
                switch (plugin->type())
                {
                case plugin_video:
                    id = IDC_COMBO_GFX;
                    break;
                case plugin_audio:
                    id = IDC_COMBO_SOUND;
                    break;
                case plugin_input:
                    id = IDC_COMBO_INPUT;
                    break;
                case plugin_rsp:
                    id = IDC_COMBO_RSP;
                    break;
                default:
                    assert(false);
                    break;
                }
                // we add the string and associate a pointer to the plugin with the item
                const int i = SendDlgItemMessage(hwnd, id, CB_GETCOUNT, 0, 0);
                SendDlgItemMessage(hwnd, id, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(io_service.string_to_wstring(plugin->name()).c_str()));
                SendDlgItemMessage(hwnd, id, CB_SETITEMDATA, i, (LPARAM)plugin.get());
            }

            update_plugin_selection(hwnd, IDC_COMBO_GFX, g_config.selected_video_plugin);
            update_plugin_selection(hwnd, IDC_COMBO_SOUND, g_config.selected_audio_plugin);
            update_plugin_selection(hwnd, IDC_COMBO_INPUT, g_config.selected_input_plugin);
            update_plugin_selection(hwnd, IDC_COMBO_RSP, g_config.selected_rsp_plugin);

            const auto ids_to_enable = {
            IDM_VIDEO_SETTINGS,
            IDM_AUDIO_SETTINGS,
            IDM_INPUT_SETTINGS,
            IDM_RSP_SETTINGS,
            IDGFXTEST,
            IDSOUNDTEST,
            IDINPUTTEST,
            IDRSPTEST,
            IDGFXABOUT,
            IDSOUNDABOUT,
            IDINPUTABOUT,
            IDRSPABOUT,
            };

            EnableWindow(GetDlgItem(hwnd, IDC_COMBO_GFX), !g_core_ctx->vr_get_launched());
            EnableWindow(GetDlgItem(hwnd, IDC_COMBO_INPUT), !g_core_ctx->vr_get_launched());
            EnableWindow(GetDlgItem(hwnd, IDC_COMBO_SOUND), !g_core_ctx->vr_get_launched());
            EnableWindow(GetDlgItem(hwnd, IDC_COMBO_RSP), !g_core_ctx->vr_get_launched());

            for (const auto& id : ids_to_enable)
            {
                EnableWindow(GetDlgItem(hwnd, id), true);
            }

            update_plugin_buttons_enabled_state(hwnd);

            break;
        }
    case WM_COMMAND:
        switch (LOWORD(w_param))
        {
        case IDC_COMBO_GFX:
        case IDC_COMBO_SOUND:
        case IDC_COMBO_INPUT:
        case IDC_COMBO_RSP:
            update_plugin_buttons_enabled_state(hwnd);
            break;
        case IDM_VIDEO_SETTINGS:
            g_hwnd_plug = hwnd;
            get_selected_plugin(hwnd, IDC_COMBO_GFX)->config();
            break;
        case IDGFXTEST:
            g_hwnd_plug = hwnd;
            get_selected_plugin(hwnd, IDC_COMBO_GFX)->test();
            break;
        case IDGFXABOUT:
            g_hwnd_plug = hwnd;
            get_selected_plugin(hwnd, IDC_COMBO_GFX)->about();
            break;
        case IDM_INPUT_SETTINGS:
            g_hwnd_plug = hwnd;
            get_selected_plugin(hwnd, IDC_COMBO_INPUT)->config();
            break;
        case IDINPUTTEST:
            g_hwnd_plug = hwnd;
            get_selected_plugin(hwnd, IDC_COMBO_INPUT)->test();
            break;
        case IDINPUTABOUT:
            g_hwnd_plug = hwnd;
            get_selected_plugin(hwnd, IDC_COMBO_INPUT)->about();
            break;
        case IDM_AUDIO_SETTINGS:
            g_hwnd_plug = hwnd;
            get_selected_plugin(hwnd, IDC_COMBO_SOUND)->config();
            break;
        case IDSOUNDTEST:
            g_hwnd_plug = hwnd;
            get_selected_plugin(hwnd, IDC_COMBO_SOUND)->test();
            break;
        case IDSOUNDABOUT:
            g_hwnd_plug = hwnd;
            get_selected_plugin(hwnd, IDC_COMBO_SOUND)->about();
            break;
        case IDM_RSP_SETTINGS:
            g_hwnd_plug = hwnd;
            get_selected_plugin(hwnd, IDC_COMBO_RSP)->config();
            break;
        case IDRSPTEST:
            g_hwnd_plug = hwnd;
            get_selected_plugin(hwnd, IDC_COMBO_RSP)->test();
            break;
        case IDRSPABOUT:
            g_hwnd_plug = hwnd;
            get_selected_plugin(hwnd, IDC_COMBO_RSP)->about();
            break;
        case IDC_PLUGIN_DISCOVERY_INFO:
            DialogBox(g_app_instance, MAKEINTRESOURCE(IDD_PLUGIN_DISCOVERY_RESULTS), hwnd, plugin_discovery_dlgproc);
            break;
        default:
            break;
        }
        break;
    case WM_NOTIFY:
        if (lpnmhdr->code == PSN_SETACTIVE)
        {
            g_config.settings_tab = 0;

            if (g_plugin_discovery_rescan)
            {
                refresh_plugins_page(hwnd);
                g_plugin_discovery_rescan = false;
            }
        }

        if (lpnmhdr->code == PSN_APPLY)
        {
            if (const auto plugin = get_selected_plugin(hwnd, IDC_COMBO_GFX); plugin != nullptr)
            {
                g_config.selected_video_plugin = plugin->path().wstring();
            }
            if (const auto plugin = get_selected_plugin(hwnd, IDC_COMBO_SOUND); plugin != nullptr)
            {
                g_config.selected_audio_plugin = plugin->path().wstring();
            }
            if (const auto plugin = get_selected_plugin(hwnd, IDC_COMBO_INPUT); plugin != nullptr)
            {
                g_config.selected_input_plugin = plugin->path().wstring();
            }
            if (const auto plugin = get_selected_plugin(hwnd, IDC_COMBO_RSP); plugin != nullptr)
            {
                g_config.selected_rsp_plugin = plugin->path().wstring();
            }
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

void get_config_listview_items(std::vector<t_options_group>& groups, std::vector<t_options_item>& options)
{
    size_t id = 0;

    t_options_group interface_group = {
    .id = id++,
    .name = L"Interface"};

    t_options_group statusbar_group = {
    .id = id++,
    .name = L"Statusbar"};

    t_options_group seek_piano_roll_group = {
    .id = id++,
    .name = L"Seek / Piano Roll"};

    t_options_group flow_group = {
    .id = id++,
    .name = L"Flow"};

    t_options_group capture_group = {
    .id = id++,
    .name = L"Capture"};

    t_options_group core_group = {
    .id = id++,
    .name = L"Core"};

    t_options_group vcr_group = {
    .id = id++,
    .name = L"VCR"};

    t_options_group lua_group = {
    .id = id++,
    .name = L"Lua"};

    t_options_group debug_group = {
    .id = id++,
    .name = L"Debug"};

    groups = {interface_group, statusbar_group, seek_piano_roll_group, flow_group, capture_group, core_group, vcr_group, lua_group, debug_group};

#define RPROP(T, x) t_options_item::t_readonly_property([] { \
    return g_default_config.x;                               \
})

#define RWPROP(T, x) t_options_item::t_readwrite_property([] {                                            \
    return g_config.x;                                                                                    \
},                                                                                                        \
                                                          [](const t_options_item::data_variant& value) { \
                                                              g_config.x = std::get<T>(value);            \
                                                          })

#define GENPROPS(T, x) .current_value = RWPROP(T, x), .default_value = RPROP(T, x)

    options = {
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = interface_group.id,
    .name = L"Pause when unfocused",
    .tooltip = L"Pause emulation when the main window isn't in focus.",
    GENPROPS(int32_t, is_unfocused_pause_enabled),
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = interface_group.id,
    .name = L"Automatic Update Checking",
    .tooltip = L"Enables automatic update checking. Requires an internet connection.",
    GENPROPS(int32_t, automatic_update_checking),
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = interface_group.id,
    .name = L"Silent Mode",
    .tooltip = L"Suppresses all dialogs and chooses reasonable defaults for multiple-choice dialogs.\nCan cause data loss during normal usage; only enable in automation scenarios!",
    GENPROPS(int32_t, silent_mode),
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = interface_group.id,
    .name = L"Keep working directory",
    .tooltip = L"Keep the working directory specified by the caller program at startup.\nWhen disabled, mupen changes the working directory to its current path.",
    GENPROPS(int32_t, keep_default_working_directory),
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = interface_group.id,
    .name = L"Async Plugin Discovery",
    .tooltip = L"Whether plugins discovery is performed asynchronously. Removes potential waiting times in the config dialog.",
    GENPROPS(int32_t, plugin_discovery_async),
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = interface_group.id,
    .name = L"Auto-increment Slot",
    .tooltip = L"Automatically increment the save slot upon saving a state.",
    GENPROPS(int32_t, increment_slot),
    },

    t_options_item{
    .type = t_options_item::Type::Enum,
    .group_id = statusbar_group.id,
    .name = L"Layout",
    .tooltip = L"The statusbar layout preset.\nClassic - The legacy layout\nModern - The new layout containing additional information\nModern+ - The new layout, but with a section for read-only status",
    GENPROPS(int32_t, statusbar_layout),
    .possible_values = {
    std::make_pair(L"Classic", (int32_t)t_config::StatusbarLayout::Classic),
    std::make_pair(L"Modern", (int32_t)t_config::StatusbarLayout::Modern),
    std::make_pair(L"Modern+", (int32_t)t_config::StatusbarLayout::ModernWithReadOnly),
    },
    },
    t_options_item{.type = t_options_item::Type::Bool, .group_id = statusbar_group.id, .name = L"Zero-index", .tooltip = L"Show indicies in the statusbar, such as VCR frame counts, relative to 0 instead of 1.", GENPROPS(int32_t, vcr_0_index)},
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = statusbar_group.id,
    .name = L"Scale down to fit window",
    .tooltip = L"Whether the statusbar is allowed to scale its segments down.",
    GENPROPS(int32_t, statusbar_scale_down),
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = statusbar_group.id,
    .name = L"Scale up to fill window",
    .tooltip = L"Whether the statusbar is allowed to scale its segments up.",
    GENPROPS(int32_t, statusbar_scale_up),
    },

    t_options_item{
    .type = t_options_item::Type::Number,
    .group_id = seek_piano_roll_group.id,
    .name = L"Savestate Interval",
    .tooltip = L"The interval at which to create savestates for seeking. Piano Roll is exclusively read-only if this value is 0.\nHigher numbers will reduce the seek duration at cost of emulator performance, a value of 1 is not allowed.\n0 - Seek savestate generation disabled\nRecommended: 100",
    GENPROPS(int32_t, core.seek_savestate_interval),
    .is_readonly = [] {
        return g_core_ctx->vcr_get_task() != task_idle;
    },
    },
    t_options_item{
    .type = t_options_item::Type::Number,
    .group_id = seek_piano_roll_group.id,
    .name = L"Savestate Max Count",
    .tooltip = L"The maximum amount of savestates to keep in memory for seeking.\nHigher numbers might cause an out of memory exception.",
    GENPROPS(int32_t, core.seek_savestate_max_count),
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = seek_piano_roll_group.id,
    .name = L"Constrain edit to column",
    .tooltip = L"Whether piano roll edits are constrained to the column they started on.",
    GENPROPS(int32_t, piano_roll_constrain_edit_to_column),
    },
    t_options_item{
    .type = t_options_item::Type::Number,
    .group_id = seek_piano_roll_group.id,
    .name = L"History size",
    .tooltip = L"Maximum size of the history list.",
    GENPROPS(int32_t, piano_roll_undo_stack_size),
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = seek_piano_roll_group.id,
    .name = L"Keep selection visible",
    .tooltip = L"Whether the piano roll will try to keep the selection visible.",
    GENPROPS(int32_t, piano_roll_keep_selection_visible),
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = seek_piano_roll_group.id,
    .name = L"Keep playhead visible",
    .tooltip = L"Whether the piano roll will try to keep the playhead visible.",
    GENPROPS(int32_t, piano_roll_keep_playhead_visible),
    },

    t_options_item{
    .type = t_options_item::Type::Number,
    .group_id = capture_group.id,
    .name = L"Delay",
    .tooltip = L"Miliseconds to wait before capturing a frame. Useful for syncing with external programs.",
    GENPROPS(int32_t, capture_delay),
    },
    t_options_item{
    .type = t_options_item::Type::Enum,
    .group_id = capture_group.id,
    .name = L"Encoder",
    .tooltip = L"The encoder to use when generating an output file.\nVFW - Slow but stable (recommended)\nFFmpeg - Fast but less stable",
    GENPROPS(int32_t, encoder_type),
    .possible_values = {
    std::make_pair(L"VFW", (int32_t)t_config::EncoderType::VFW),
    std::make_pair(L"FFmpeg (experimental)", (int32_t)t_config::EncoderType::FFmpeg),
    },
    .is_readonly = [] {
        return EncodingManager::is_capturing();
    },
    },
    t_options_item{
    .type = t_options_item::Type::Enum,
    .group_id = capture_group.id,
    .name = L"Mode",
    .tooltip = L"The video source to use for capturing video frames.\nPlugin - Captures frames solely from the video plugin\nWindow - Captures frames from the main window\nScreen - Captures screenshots of the current display and crops them to Mupen\nHybrid - Combines video plugin capture and internal Lua composition (recommended)",
    GENPROPS(int32_t, capture_mode),
    .possible_values = {
    std::make_pair(L"Plugin", 0),
    std::make_pair(L"Window", 1),
    std::make_pair(L"Screen", 2),
    std::make_pair(L"Hybrid", 3),
    },
    .is_readonly = [] {
        return EncodingManager::is_capturing();
    },
    },
    t_options_item{
    .type = t_options_item::Type::Enum,
    .group_id = capture_group.id,
    .name = L"Sync",
    .tooltip = L"The strategy to use for synchronizing video and audio during capture.\nNone - No synchronization\nAudio - Audio is synchronized to video\nVideo - Video is synchronized to audio",
    GENPROPS(int32_t, synchronization_mode),
    .possible_values = {
    std::make_pair(L"None", 0),
    std::make_pair(L"Audio", 1),
    std::make_pair(L"Video", 2),
    },
    .is_readonly = [] {
        return EncodingManager::is_capturing();
    },
    },
    t_options_item{
    .type = t_options_item::Type::String,
    .group_id = capture_group.id,
    .name = L"FFmpeg Path",
    .tooltip = L"The path to the FFmpeg executable to use for capturing.",
    GENPROPS(std::wstring, ffmpeg_path),
    .is_readonly = [] {
        return EncodingManager::is_capturing();
    },
    },
    t_options_item{
    .type = t_options_item::Type::String,
    .group_id = capture_group.id,
    .name = L"FFmpeg Arguments",
    .tooltip = L"The argument format string to be passed to FFmpeg when capturing.",
    GENPROPS(std::wstring, ffmpeg_final_options),
    .is_readonly = [] {
        return EncodingManager::is_capturing();
    },
    },

    t_options_item{
    .type = t_options_item::Type::Enum,
    .group_id = core_group.id,
    .name = L"Type",
    .tooltip = L"The core type to utilize for emulation.\nInterpreter - Slow and relatively accurate\nDynamic Recompiler - Fast, possibly less accurate, and only for x86 processors\nPure Interpreter - Very slow and accurate",
    GENPROPS(int32_t, core.core_type),
    .possible_values = {
    std::make_pair(L"Interpreter", 0),
    std::make_pair(L"Dynamic Recompiler", 1),
    std::make_pair(L"Pure Interpreter", 2),
    },
    .is_readonly = [] {
        return g_core_ctx->vr_get_launched();
    },
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = core_group.id,
    .name = L"Undo Savestate Load",
    .tooltip = L"Whether undo savestate load functionality is enabled.",
    GENPROPS(int32_t, core.st_undo_load),
    },
    t_options_item{
    .type = t_options_item::Type::Number,
    .group_id = core_group.id,
    .name = L"Counter Factor",
    .tooltip = L"The CPU's counter factor.\nValues above 1 are effectively 'lagless'.",
    GENPROPS(int32_t, core.counter_factor),
    },
    t_options_item{
    .type = t_options_item::Type::Number,
    .group_id = core_group.id,
    .name = L"Max Lag Frames",
    .tooltip = L"The maximum amount of lag frames before the core emits a warning\n0 - Disabled",
    GENPROPS(int32_t, core.max_lag),
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = core_group.id,
    .name = L"WiiVC Mode",
    .tooltip = L"Enables WiiVC emulation.",
    GENPROPS(int32_t, core.wii_vc_emulation),
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = core_group.id,
    .name = L"Emulate Float Crashes",
    .tooltip = L"Emulate float operation-related crashes which would also crash on real hardware",
    GENPROPS(int32_t, core.float_exception_emulation),
    },
    t_options_item{
    .type = t_options_item::Type::Number,
    .group_id = core_group.id,
    .name = L"Fast-Forward Skip Frequency",
    .tooltip = L"Skip rendering every nth frame when in fast-forward mode.\n0 - Render nothing\n1 - Render every frame\nn - Render every nth frame",
    GENPROPS(int32_t, core.frame_skip_frequency),
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = core_group.id,
    .name = L"Emulate SD Card",
    .tooltip = L"Enable SD card emulation.\nRequires a VHD-formatted SD card file named card.vhd in the same folder as Mupen.",
    GENPROPS(int32_t, core.use_summercart),
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = core_group.id,
    .name = L"Instant Savestate Update",
    .tooltip = L"Saves and loads game graphics to savestates to allow instant graphics updates when loading savestates.\nGreatly increases savestate saving and loading time.",
    GENPROPS(int32_t, core.st_screenshot),
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = core_group.id,
    .name = L"Skip rendering lag",
    .tooltip = L"Prevents calls to updateScreen during lag.\nMight improve performance on some video plugins at the cost of stability.",
    GENPROPS(int32_t, core.skip_rendering_lag),
    },
    t_options_item{
    .type = t_options_item::Type::Number,
    .group_id = core_group.id,
    .name = L"ROM Cache Size",
    .tooltip = L"Size of the ROM cache.\nImproves ROM loading performance at the cost of data staleness and high memory usage.\n0 - Disabled\nn - Maximum of n ROMs kept in cache",
    GENPROPS(int32_t, core.rom_cache_size),
    },

    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = vcr_group.id,
    .name = L"Movie Backups",
    .tooltip = L"Generate a backup of the currently recorded movie when loading a savestate.\nBackups are saved in the backups folder.",
    GENPROPS(int32_t, core.vcr_backups),
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = vcr_group.id,
    .name = L"Extended Movie Format",
    .tooltip = L"Whether movies are written using the new extended format.\nUseful when opening movies in external programs which don't handle the new format correctly.\nIf disabled, the extended format sections are set to 0.",
    GENPROPS(int32_t, core.vcr_write_extended_format),
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = vcr_group.id,
    .name = L"Record Resets",
    .tooltip = L"Record manually performed resets to the current movie.\nThese resets will be repeated when the movie is played back.",
    GENPROPS(int32_t, core.is_reset_recording_enabled),
    },

    t_options_item{
    .type = t_options_item::Type::Enum,
    .group_id = lua_group.id,
    .name = L"Presenter",
    .tooltip = L"The presenter type to use for displaying and capturing Lua graphics.\nRecommended: DirectComposition",
    GENPROPS(int32_t, presenter_type),
    .possible_values = {
    std::make_pair(L"DirectComposition", (int32_t)t_config::PresenterType::DirectComposition),
    std::make_pair(L"GDI", (int32_t)t_config::PresenterType::GDI),
    },
    .is_readonly = [] {
        return !g_lua_environments.empty();
    },
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = lua_group.id,
    .name = L"Lazy Renderer Initialization",
    .tooltip = L"Enables lazy Lua renderer initialization. Greatly speeds up start and stop times for certain scripts.",
    GENPROPS(int32_t, lazy_renderer_init),
    .is_readonly = [] {
        return !g_lua_environments.empty();
    },
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = lua_group.id,
    .name = L"Fast Dispatcher",
    .tooltip = L"Enables a low-latency dispatcher implementation. Can improve performance with Lua scripts.\nDisable if the UI is stuttering heavily or if you're using a low-end machine.",
    GENPROPS(int32_t, fast_dispatcher),
    },

    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = debug_group.id,
    .name = L"Audio Delay",
    .tooltip = L"Whether to delay audio interrupts.",
    GENPROPS(int32_t, core.is_audio_delay_enabled),
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = debug_group.id,
    .name = L"Compiled Jump",
    .tooltip = L"Whether the Dynamic Recompiler core compiles jumps.",
    GENPROPS(int32_t, core.is_compiled_jump_enabled),
    },
    t_options_item{
    .type = t_options_item::Type::Bool,
    .group_id = debug_group.id,
    .name = L"Accurate C.EQ.S",
    .tooltip = L"Whether the C_EQ_S instruction produces `(NaN == any) == false` when using the Dynamic Recompiler core.\nThe legacy behaviour is `(NaN == any) == true`, but this option is kept for backwards-compatibility.",
    GENPROPS(int32_t, core.c_eq_s_nan_accurate),
    .is_readonly = [] {
        return g_core_ctx->vr_get_launched();
    },
    },
    };
}

LRESULT CALLBACK inline_edit_subclass_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR ref_data)
{
    switch (msg)
    {
    case WM_GETDLGCODE:
        {
            if (wparam == VK_RETURN)
            {
                goto apply;
            }
            if (wparam == VK_ESCAPE)
            {
                DestroyWindow(hwnd);
            }
            break;
        }
    case WM_KILLFOCUS:
        goto apply;
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, inline_edit_subclass_proc, id);
        g_edit_hwnd = nullptr;
        break;
    default:
        break;
    }

def:
    return DefSubclassProc(hwnd, msg, wparam, lparam);

apply:

    const auto text = get_window_text(hwnd).value_or(L"");
    SendMessage(GetParent(hwnd), WM_EDIT_END, 0, (LPARAM)text.c_str());

    DestroyWindow(hwnd);

    goto def;
}

INT_PTR CALLBACK edit_string_dlgproc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        {
            const auto option_item = g_option_items[g_edit_option_item_index];
            const auto edit_hwnd = GetDlgItem(wnd, IDC_TEXTBOX_LUAPROMPT);

            const auto current_value = std::get<std::wstring>(option_item.current_value.get());

            SetWindowText(wnd, std::format(L"Edit '{}'", option_item.name).c_str());
            Edit_SetText(edit_hwnd, current_value.c_str());

            SetFocus(GetDlgItem(wnd, IDC_TEXTBOX_LUAPROMPT));
            break;
        }
    case WM_CLOSE:
        EndDialog(wnd, IDCANCEL);
        break;
    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDOK:
            {
                const auto& option_item = g_option_items[g_edit_option_item_index];
                const auto edit_hwnd = GetDlgItem(wnd, IDC_TEXTBOX_LUAPROMPT);

                const auto str = get_window_text(edit_hwnd).value_or(L"");

                option_item.current_value.set(str);

                EndDialog(wnd, IDOK);
                break;
            }
        case IDCANCEL:
            EndDialog(wnd, IDCANCEL);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

/**
 * Advances a listview's selection by one.
 */
void advance_listview_selection(HWND lvhwnd)
{
    int32_t i = ListView_GetNextItem(lvhwnd, -1, LVNI_SELECTED);
    if (i == -1)
        return;
    ListView_SetItemState(lvhwnd, i, 0, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_SetItemState(lvhwnd, i + 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(lvhwnd, i + 1, false);
}

bool begin_settings_lv_edit(HWND hwnd, int i)
{
    auto option_item = g_option_items[i];

    // TODO: Perhaps gray out readonly values too?
    if (option_item.is_readonly())
    {
        return false;
    }

    // For bools, just flip the value...
    if (option_item.type == t_options_item::Type::Bool)
    {
        option_item.current_value.set(!std::get<int32_t>(option_item.current_value.get()));
    }

    // For enums, cycle through the possible values
    if (option_item.type == t_options_item::Type::Enum)
    {
        // 1. Find the index of the currently selected item, while falling back to the first possible value if there's no match
        int32_t current_value = option_item.possible_values[0].second;
        for (const auto& [_, possible_value] : option_item.possible_values)
        {
            if (std::get<int32_t>(option_item.current_value.get()) == possible_value)
            {
                current_value = possible_value;
                break;
            }
        }

        // 2. Find the lowest and highest values in the vector
        int32_t min_possible_value = INT32_MAX;
        int32_t max_possible_value = INT32_MIN;
        for (const auto& [_, val] : option_item.possible_values)
        {
            max_possible_value = std::max(val, max_possible_value);
            if (val < min_possible_value)
            {
                min_possible_value = val;
            }
        }

        // 2. Bump it, wrapping around if needed
        current_value++;
        if (current_value > max_possible_value)
        {
            current_value = min_possible_value;
        }

        // 3. Apply the change
        option_item.current_value.set(current_value);
    }

    // For strings, allow editing in a dialog (since it might be a multiline string and we can't really handle that below)
    if (option_item.type == t_options_item::Type::String)
    {
        g_edit_option_item_index = i;
        DialogBoxParam(g_app_instance, MAKEINTRESOURCE(IDD_LUAINPUTPROMPT), hwnd, edit_string_dlgproc, 0);
    }

    // For numbers, create a textbox over the value cell for inline editing
    if (option_item.type == t_options_item::Type::Number)
    {
        if (g_edit_hwnd)
        {
            DestroyWindow(g_edit_hwnd);
        }

        g_edit_option_item_index = i;

        RECT item_rect{};
        ListView_GetSubItemRect(g_lv_hwnd, i, 1, LVIR_LABEL, &item_rect);

        RECT lv_rect{};
        GetClientRect(g_lv_hwnd, &lv_rect);

        item_rect.right = lv_rect.right;

        g_edit_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP, item_rect.left, item_rect.top, item_rect.right - item_rect.left, item_rect.bottom - item_rect.top, hwnd, 0, g_app_instance, 0);

        SendMessage(g_edit_hwnd, WM_SETFONT, (WPARAM)SendMessage(g_lv_hwnd, WM_GETFONT, 0, 0), 0);

        SetWindowSubclass(g_edit_hwnd, inline_edit_subclass_proc, 0, 0);

        const auto value = std::get<int32_t>(option_item.current_value.get());
        Edit_SetText(g_edit_hwnd, std::to_wstring(value).c_str());

        PostMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)g_edit_hwnd, TRUE);
    }

    // For hotkeys, accept keyboard inputs
    if (option_item.type == t_options_item::Type::Hotkey)
    {
        auto hotkey = std::get<Hotkey::t_hotkey>(option_item.current_value.get());

        Hotkey::show_prompt(hwnd, std::format(L"Choose a hotkey for {}", option_item.name), hotkey);

        advance_listview_selection(g_lv_hwnd);

        Hotkey::try_associate_hotkey(hwnd, option_item.name, hotkey, false);

        ListView_RedrawItems(g_lv_hwnd, 0, ListView_GetItemCount(g_lv_hwnd));
    }

    ListView_Update(g_lv_hwnd, i);
    return true;
}

INT_PTR CALLBACK general_cfg(const HWND hwnd, const UINT message, const WPARAM w_param, const LPARAM l_param)
{
    const auto lpnmhdr = reinterpret_cast<LPNMHDR>(l_param);

    switch (message)
    {
    case WM_INITDIALOG:
        {
            if (g_lv_hwnd)
            {
                DestroyWindow(g_lv_hwnd);
            }

            RECT grid_rect{};
            GetClientRect(hwnd, &grid_rect);

            std::vector<std::wstring> groups;
            for (const auto& group : g_option_groups)
            {
                groups.push_back(group.name);
            }

            std::vector<std::pair<size_t, std::wstring>> items;
            for (const auto& item : g_option_items)
            {
                items.emplace_back(item.group_id, item.name);
            }

            auto get_item_tooltip = [](size_t i) {
                return g_option_items[i].tooltip;
            };

            auto edit_start = [=](size_t i) {
                begin_settings_lv_edit(hwnd, i);
            };

            auto get_item_image = [](size_t i) {
                const auto& option_item = g_option_items[i];

                int32_t image = option_item.initial_value.get() == option_item.current_value.get() ? 50 : 1;

                if (option_item.is_readonly())
                {
                    image = 0;
                }

                return image;
            };

            auto get_item_text = [](size_t i, size_t subitem) {
                if (subitem == 0)
                {
                    return g_option_items[i].get_name();
                }

                return g_option_items[i].get_value_name();
            };

            g_lv_hwnd = SettingsListView::create({
            .dlg_hwnd = hwnd,
            .rect = grid_rect,
            .on_edit_start = edit_start,
            .groups = groups,
            .items = items,
            .get_item_tooltip = get_item_tooltip,
            .get_item_text = get_item_text,
            .get_item_image = get_item_image,
            });

            return TRUE;
        }
    case WM_EDIT_END:
        {
            auto option_item = g_option_items[g_edit_option_item_index];
            auto str = reinterpret_cast<wchar_t*>(l_param);

            if (option_item.type == t_options_item::Type::Number)
            {
                try
                {
                    int32_t result = std::stoi(str);
                    option_item.current_value.set(result);
                }
                catch (...)
                {
                    // ignored
                }
            }
            else
            {
                option_item.current_value.set(std::wstring(str));
            }

            ListView_Update(g_lv_hwnd, g_edit_option_item_index);

            break;
        }
    case WM_CONTEXTMENU:
        {
            int32_t i = ListView_GetNextItem(g_lv_hwnd, -1, LVNI_SELECTED);

            if (i == -1)
                break;

            LVITEM item = {0};
            item.mask = LVIF_PARAM;
            item.iItem = i;
            ListView_GetItem(g_lv_hwnd, &item);

            auto option_item = g_option_items[item.lParam];
            auto readonly = option_item.is_readonly();

            HMENU h_menu = CreatePopupMenu();
            AppendMenu(h_menu, MF_STRING | (readonly ? MF_DISABLED : MF_ENABLED), 1, L"Reset to default");
            AppendMenu(h_menu, MF_STRING, 2, L"More info...");
            AppendMenu(h_menu, MF_SEPARATOR, 100, L"");
            if (option_item.type == t_options_item::Type::Hotkey)
            {
                AppendMenu(h_menu, MF_STRING, 3, L"Clear");
                AppendMenu(h_menu, MF_SEPARATOR, 101, L"");
            }
            AppendMenu(h_menu, MF_STRING, 5, L"Reset all to default");

            const int offset = TrackPopupMenuEx(h_menu, TPM_RETURNCMD | TPM_NONOTIFY, GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param), hwnd, 0);

            if (offset < 0)
            {
                break;
            }

            switch (offset)
            {
            case 1:
                option_item.reset_to_default();
                ListView_Update(g_lv_hwnd, i);
                break;
            case 2:
                DialogService::show_dialog(option_item.get_friendly_info().c_str(), option_item.name.c_str(), fsvc_information, hwnd);
                break;
            case 3:
                option_item.current_value.set(Hotkey::t_hotkey{});
                ListView_Update(g_lv_hwnd, i);
                break;
            case 5:
                {
                    // If some settings can't be changed, we'll bail
                    bool can_all_be_changed = true;

                    for (const auto& item : g_option_items)
                    {
                        if (!item.is_readonly())
                            continue;

                        can_all_be_changed = false;
                        break;
                    }

                    if (!can_all_be_changed)
                    {
                        DialogService::show_dialog(L"Some settings can't be reset, as they are currently read-only. Try again with emulation stopped.\nNo changes have been made to the settings.", L"Reset all to default", fsvc_warning, hwnd);
                        break;
                    }

                    const auto result = DialogService::show_ask_dialog(VIEW_DLG_RESET_SETTINGS, L"Are you sure you want to reset all settings to default?", L"Reset all to default", false, hwnd);

                    if (!result)
                    {
                        break;
                    }

                    for (auto& v : g_option_items)
                    {
                        v.reset_to_default();
                    }

                    ListView_RedrawItems(g_lv_hwnd, 0, ListView_GetItemCount(g_lv_hwnd));
                    break;
                }
            default:
                break;
            }

            DestroyMenu(h_menu);
        }
        break;
    case WM_NOTIFY:
        {
            if (lpnmhdr->code == PSN_SETACTIVE)
            {
                g_config.settings_tab = 2;
            }

            return SettingsListView::notify(hwnd, g_lv_hwnd, l_param, w_param);
        }
    default:
        return FALSE;
    }
    return TRUE;
}

void ConfigDialog::init()
{
    get_config_listview_items(g_option_groups, g_option_items);
}
/**
 * \brief Generate option groups with names based on the path segments
 * e.g.:
 * Mupen64 > File > Load ROM... is grouped under "Mupen64 > File"
 * Mupen64 > Emulation > Pause is grouped under "Mupen64 > Emulation"
 * Mupen64 > Emulation > Frame Advance is grouped under "Mupen64 > Emulation"
 * SM64Lua > Match Yaw is grouped under "SM64Lua"
 */
static std::vector<t_options_group> generate_hotkey_groups(size_t base_id)
{
    std::vector<std::wstring> unique_group_names;
    const auto all_actions = ActionManager::get_actions_matching_filter(L"*");

    for (const auto& path : all_actions)
    {
        std::vector<std::wstring> segments = ActionManager::get_segments(path);

        if (segments.size() <= 1)
        {
            continue;
        }

        segments.pop_back();

        std::wstring group_name;
        for (size_t i = 0; i < segments.size(); ++i)
        {
            if (i > 0)
            {
                group_name += ActionManager::SEGMENT_SEPARATOR;
            }
            group_name += segments[i];
        }

        if (std::ranges::find(unique_group_names, group_name) == unique_group_names.end())
        {
            unique_group_names.emplace_back(group_name);
        }
    }

    std::vector<t_options_group> groups;
    groups.reserve(unique_group_names.size());

    for (const auto& name : unique_group_names)
    {
        groups.emplace_back(t_options_group{
        .id = base_id++,
        .name = name});
    }

    return groups;
}


void ConfigDialog::show_app_settings()
{
    const auto prev_option_group_size = g_option_groups.size();
    const auto prev_option_items_size = g_option_items.size();

    auto option_groups = generate_hotkey_groups(g_option_groups.back().id + 1);

    for (const auto& group : option_groups)
    {
        const auto actions = ActionManager::get_actions_matching_filter(std::format(L"{} > *", group.name));

        for (const auto& action : actions)
        {
            const auto action_segments = ActionManager::get_segments(action);
            const auto group_segments = ActionManager::get_segments(group.name);

            if (action_segments.at(action_segments.size() - 2) != group_segments.back())
            {
                continue;
            }

            const t_options_item item = {
            .type = t_options_item::Type::Hotkey,
            .group_id = group.id,
            .name = action,
            .current_value = t_options_item::t_readwrite_property([=] {
                return g_config.hotkeys.at(action);
            },
                                                                  [=](const t_options_item::data_variant& value) {
                                                                      g_config.hotkeys[action] = std::get<Hotkey::t_hotkey>(value);
                                                                  }),
            .default_value = t_options_item::t_readonly_property([=] {
                return g_config.inital_hotkeys.at(action);
            }),
            };

            g_option_items.emplace_back(item);
        }
    }

    for (auto& option_item : g_option_items)
    {
        const auto initial_value = option_item.current_value.get();
        option_item.initial_value = t_options_item::t_readonly_property([=] {
            return initial_value;
        });
    }

    // We beautify the names here, a bit annoying because we have to reconstruct them
    for (auto& option_group : option_groups)
    {
        auto segments = ActionManager::get_segments(option_group.name);
        for (auto& segment : segments)
        {
            segment = ActionManager::get_display_name(segment, true);
        }
        const auto name = io_service.join_wstring(segments, std::format(L" {} ", ActionManager::SEGMENT_SEPARATOR));
        option_group.name = name;
    }

    g_option_groups.insert(g_option_groups.end(), option_groups.begin(), option_groups.end());

    PROPSHEETPAGE psp[3] = {{0}};
    for (auto& i : psp)
    {
        i.dwSize = sizeof(PROPSHEETPAGE);
        i.dwFlags = PSP_USETITLE;
        i.hInstance = g_app_instance;
    }

    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_SETTINGS_PLUGINS);
    psp[0].pfnDlgProc = plugins_cfg;
    psp[0].pszTitle = L"Plugins";

    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_DIRECTORIES);
    psp[1].pfnDlgProc = directories_cfg;
    psp[1].pszTitle = L"Directories";

    psp[2].pszTemplate = MAKEINTRESOURCE(IDD_SETTINGS_GENERAL);
    psp[2].pfnDlgProc = general_cfg;
    psp[2].pszTitle = L"General";

    PROPSHEETHEADER psh = {0};
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
    psh.hwndParent = g_main_hwnd;
    psh.hInstance = g_app_instance;
    psh.pszCaption = L"Settings";
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = g_config.settings_tab;
    psh.ppsp = (LPCPROPSHEETPAGE)&psp;

    g_prev_config = g_config;

    const bool cancelled = !PropertySheet(&psh);

    if (cancelled)
    {
        g_config = g_prev_config;
    }

    ActionManager::begin_batch_work();
    for (const auto& [action, hotkey] : g_config.hotkeys)
    {
        ActionManager::associate_hotkey(action, hotkey, true);
    }
    ActionManager::end_batch_work();

    g_option_items.erase(g_option_items.begin() + prev_option_items_size, g_option_items.end());
    g_option_groups.erase(g_option_groups.begin() + prev_option_group_size, g_option_groups.end());

    Config::save();
    Messenger::broadcast(Messenger::Message::ConfigLoaded, nullptr);
}
