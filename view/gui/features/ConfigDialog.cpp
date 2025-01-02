/***************************************************************************
                                                  configdialog.c  -  description
                                                         -------------------
        copyright            : (C) 2003 by ShadowPrince
        email                : shadow@emulation64.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "configdialog.h"
#include <cassert>
#include <core/r4300/Plugin.hpp>
#include <core/r4300/r4300.h>
#include <core/r4300/timers.h>
#include <cstdio>
#include <shared/Config.hpp>
#include <shared/helpers/StlExtensions.h>
#include <shlobj.h>
#include <vector>
#include <view/capture/EncodingManager.h>
#include <view/gui/Main.h>
#include <view/gui/wrapper/PersistentPathDialog.h>
#include <view/helpers/WinHelpers.h>
#include <view/lua/LuaConsole.h>
#include <view/resource.h>

#include <Windows.h>
#include <shared/Messenger.h>

#include "core/r4300/vcr.h"
#include "shared/services/FrontendService.h"

#define WM_EDIT_END (WM_USER + 19)
#define WM_PLUGIN_DISCOVERY_FINISHED (WM_USER + 22)

/**
 * Represents a group of options in the settings.
 */
typedef struct {
    /**
     * The group's unique identifier.
     */
    size_t id;

    /**
     * The group's name.
     */
    std::wstring name;
} t_options_group;

/**
 * Represents a settings option.
 */
typedef struct OptionsItem {
    enum class Type {
        Invalid,
        Bool,
        Number,
        Enum,
        String,
        Hotkey,
    };

    /**
     * The group this option belongs to.
     */
    size_t group_id = -1;

    /**
     * The option's name.
     */
    std::wstring name;

    /**
     * The option's tooltip.
     */
    std::wstring tooltip;

    /**
     * The option's backing data.
     */
    int32_t* data = nullptr;

    /**
     * The option's backing data as a string, used when type == Type::String
     */
    std::wstring* data_str;

    /**
     * The option's backing data type.
     */
    Type type = Type::Invalid;

    /**
     * Possible predefined values (e.g.: enum values) for an option along with a
     * name.
     *
     * Only applicable when <c>type == Type::Enum<c>.
     */
    std::vector<std::pair<std::wstring, int32_t>> possible_values = {};

    /**
     * Function which returns whether the option can be changed. Useful for values
     * which shouldn't be changed during emulation.
     */
    std::function<bool()> is_readonly = [] { return false; };

    /**
     * Gets the value name for the current backing data, or a fallback name if no
     * match is found.
     */
    std::wstring get_value_name() const
    {
        if (type == Type::Bool)
        {
            return *data ? L"On" : L"Off";
        }

        if (type == Type::Number)
        {
            return std::to_wstring(*data);
        }

        if (type == Type::String)
        {
            return *data_str;
        }

        if (type == Type::Hotkey)
        {
            auto hotkey_ptr = reinterpret_cast<t_hotkey*>(data);
            return hotkey_to_string(hotkey_ptr);
        }

        for (auto [name, val] : possible_values)
        {
            if (*data == val)
            {
                return name;
            }
        }

        return std::format(L"Unknown value ({})", *data);
    }

    /**
     * Gets the option's default value from another config struct.
     */
    void* get_default_value_ptr(const t_config* config) const
    {
        // Find the field offset for the option relative to g_config and grab the
        // equivalent from the default config
        size_t field_offset;
        if (type == Type::String)
        {
            field_offset = (char*)data_str - (char*)&g_config;
        }
        else
        {
            field_offset = (char*)data - (char*)&g_config;
        }

        if (type == Type::String)
        {
            return (std::string*)((char*)config + field_offset);
        }

        return (int32_t*)((char*)config + field_offset);
    }

    /**
     * Resets the value of the option to the default value.
     */
    void reset_to_default()
    {
        void* default_equivalent = get_default_value_ptr(&g_default_config);

        if (type == Type::String)
        {
            *data_str = *(std::wstring*)default_equivalent;
        }
        else if (type == Type::Hotkey)
        {
            auto default_hotkey = (t_hotkey*)default_equivalent;
            auto current_hotkey = (t_hotkey*)data;
            current_hotkey->key = default_hotkey->key;
            current_hotkey->ctrl = default_hotkey->ctrl;
            current_hotkey->alt = default_hotkey->alt;
            current_hotkey->shift = default_hotkey->shift;
        }
        else
        {
            *data = *(int32_t*)default_equivalent;
        }
    }

} t_options_item;

t_plugin_discovery_result plugin_discovery_result;
std::vector<HWND> tooltips;
std::vector<t_options_group> g_option_groups;
std::vector<t_options_item> g_option_items;
HWND g_lv_hwnd;
HWND g_edit_hwnd;
size_t g_edit_option_item_index;
t_config g_prev_config;

// Index of the hotkey currently being entered, if any
std::optional<size_t> g_hotkey_active_index;

std::thread g_plugin_discovery_thread;

// Whether a plugin rescan is needed. Set when modifying the plugin path.
bool g_plugin_discovery_rescan = false;

/// <summary>
/// Waits until the user inputs a valid key sequence, then fills out the hotkey
/// </summary>
/// <returns>
/// Whether a hotkey has successfully been picked
/// </returns>
int32_t get_user_hotkey(t_hotkey* hotkey)
{
    MSG msg;
    int i, j;
    int lc = 0, ls = 0, la = 0;
    for (i = 0; i < 500; i++)
    {
        SleepEx(10, TRUE);
        for (j = 8; j < 254; j++)
        {
            if (j == VK_LCONTROL || j == VK_RCONTROL || j == VK_LMENU || j == VK_RMENU || j == VK_LSHIFT || j == VK_RSHIFT)
                continue;

            if (GetAsyncKeyState(VK_RBUTTON))
            {
                return 1;
            }

            if (GetAsyncKeyState(j) & 0x8000)
            {
                // HACK to avoid exiting all the way out of the dialog on pressing
                // escape to clear a hotkeys or continually re-activating the button on
                // trying to assign space as a hotkeys
                if (j == VK_ESCAPE)
                    return 0;

                if (j == VK_CONTROL)
                {
                    lc = 1;
                    continue;
                }
                if (j == VK_SHIFT)
                {
                    ls = 1;
                    continue;
                }
                if (j == VK_MENU)
                {
                    la = 1;
                    continue;
                }
                if (j != VK_ESCAPE)
                {
                    hotkey->key = j;
                    hotkey->shift = GetAsyncKeyState(VK_SHIFT) ? 1 : 0;
                    hotkey->ctrl = GetAsyncKeyState(VK_CONTROL) ? 1 : 0;
                    hotkey->alt = GetAsyncKeyState(VK_MENU) ? 1 : 0;
                    while (PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
                        ;

                    return 1;
                }

                hotkey->key = 0;
                hotkey->shift = 0;
                hotkey->ctrl = 0;
                hotkey->alt = 0;
                while (PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
                    ;

                return 0;
            }
            if (j == VK_CONTROL && lc)
            {
                hotkey->key = 0;
                hotkey->shift = 0;
                hotkey->ctrl = 1;
                hotkey->alt = 0;
                while (PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
                    ;

                return 1;
            }
            if (j == VK_SHIFT && ls)
            {
                hotkey->key = 0;
                hotkey->shift = 1;
                hotkey->ctrl = 0;
                hotkey->alt = 0;
                while (PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
                    ;

                return 1;
            }
            if (j == VK_MENU && la)
            {
                hotkey->key = 0;
                hotkey->shift = 0;
                hotkey->ctrl = 0;
                hotkey->alt = 1;
                while (PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
                    ;

                return 1;
            }
        }
    }
    // we checked all keys and none of them was pressed, so give up
    return 0;
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

            ListView_SetExtendedListViewStyle(g_pldlv_hwnd, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

            LVCOLUMN lv_column = {0};
            lv_column.mask = LVCF_FMT | LVCF_DEFAULTWIDTH | LVCF_TEXT | LVCF_SUBITEM;

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
                    lv_item.lParam = i;
                    lv_item.iItem = i;
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

INT_PTR CALLBACK about_dlg_proc(const HWND hwnd, const UINT message, const WPARAM w_param, LPARAM)
{
    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hwnd, IDC_VERSION_TEXT, MUPEN_VERSION);
        break;
    case WM_CLOSE:
        EndDialog(hwnd, IDOK);
        break;

    case WM_COMMAND:
        switch (LOWORD(w_param))
        {
        case IDOK:
            EndDialog(hwnd, IDOK);
            break;

        case IDC_WEBSITE:
            ShellExecute(nullptr, nullptr, L"http://mupen64.emulation64.com", nullptr, nullptr, SW_SHOW);
            break;
        case IDC_GITREPO:
            ShellExecute(nullptr, nullptr, L"https://github.com/mkdasher/mupen64-rr-lua-/", nullptr, nullptr, SW_SHOW);
            break;
        default:
            break;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

void configdialog_about() { DialogBox(g_app_instance, MAKEINTRESOURCE(IDD_ABOUT), g_main_hwnd, about_dlg_proc); }

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

        if (emu_launched)
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
                const auto path = show_persistent_folder_dialog(L"f_roms", hwnd);
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
                const auto path = show_persistent_folder_dialog(L"f_plugins", hwnd);
                if (path.empty())
                {
                    break;
                }
                g_config.plugins_directory = path + L"\\";
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
                const auto path = show_persistent_folder_dialog(L"f_saves", hwnd);
                if (path.empty())
                {
                    break;
                }
                g_config.saves_directory = path + L"\\";
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
                const auto path = show_persistent_folder_dialog(L"f_screenshots", hwnd);
                if (path.empty())
                {
                    break;
                }
                g_config.screenshots_directory = path + L"\\";
                SetDlgItemText(hwnd, IDC_SCREENSHOTS_DIR, g_config.screenshots_directory.c_str());
            }
            break;
        case IDC_CHOOSE_BACKUPS_DIR:
            {
                const auto path = show_persistent_folder_dialog(L"f_backups", hwnd);
                if (path.empty())
                {
                    break;
                }
                g_config.backups_directory = path + L"\\";
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
    plugin_discovery_result = do_plugin_discovery();

    PostMessage(hwnd, WM_PLUGIN_DISCOVERY_FINISHED, 0, 0);
}

static void refresh_plugins_page(const HWND hwnd)
{
    g_view_logger->trace("[ConfigDialog] refresh_plugins_page");

    if (g_config.plugin_discovery_async)
    {
        SetDlgItemText(hwnd, IDC_PLUGIN_WARNING, L"Discovering plugins...");

        if (g_plugin_discovery_thread.joinable())
        {
            g_plugin_discovery_thread.join();
        }

        g_plugin_discovery_thread = std::thread([=] { start_plugin_discovery(hwnd); });
    }
    else
    {
        start_plugin_discovery(hwnd);
    }
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

            std::ranges::copy_if(plugin_discovery_result.results, std::back_inserter(broken_plugins), [](const auto& pair) { return !pair.second.empty(); });

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
                case PluginType::Video:
                    id = IDC_COMBO_GFX;
                    break;
                case PluginType::Audio:
                    id = IDC_COMBO_SOUND;
                    break;
                case PluginType::Input:
                    id = IDC_COMBO_INPUT;
                    break;
                case PluginType::RSP:
                    id = IDC_COMBO_RSP;
                    break;
                default:
                    assert(false);
                    break;
                }
                // we add the string and associate a pointer to the plugin with the item
                const int i = SendDlgItemMessage(hwnd, id, CB_GETCOUNT, 0, 0);
                SendDlgItemMessage(hwnd, id, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(string_to_wstring(plugin->name()).c_str()));
                SendDlgItemMessage(hwnd, id, CB_SETITEMDATA, i, (LPARAM)plugin.get());
            }

            update_plugin_selection(hwnd, IDC_COMBO_GFX, g_config.selected_video_plugin);
            update_plugin_selection(hwnd, IDC_COMBO_SOUND, g_config.selected_audio_plugin);
            update_plugin_selection(hwnd, IDC_COMBO_INPUT, g_config.selected_input_plugin);
            update_plugin_selection(hwnd, IDC_COMBO_RSP, g_config.selected_rsp_plugin);

            const auto ids_to_enable = {
            IDM_VIDEO_SETTINGS, IDM_AUDIO_SETTINGS, IDM_INPUT_SETTINGS, IDM_RSP_SETTINGS, IDGFXTEST, IDSOUNDTEST, IDINPUTTEST, IDRSPTEST, IDGFXABOUT, IDSOUNDABOUT, IDINPUTABOUT, IDRSPABOUT,
            };

            EnableWindow(GetDlgItem(hwnd, IDC_COMBO_GFX), !emu_launched);
            EnableWindow(GetDlgItem(hwnd, IDC_COMBO_INPUT), !emu_launched);
            EnableWindow(GetDlgItem(hwnd, IDC_COMBO_SOUND), !emu_launched);
            EnableWindow(GetDlgItem(hwnd, IDC_COMBO_RSP), !emu_launched);

            for (const auto& id : ids_to_enable)
            {
                EnableWindow(GetDlgItem(hwnd, id), true);
            }
            break;
        }
    case WM_COMMAND:
        switch (LOWORD(w_param))
        {
        case IDC_COMBO_GFX:
        case IDC_COMBO_SOUND:
        case IDC_COMBO_INPUT:
        case IDC_COMBO_RSP:
            {
                auto has_plugin_selected = ComboBox_GetItemData(GetDlgItem(hwnd, LOWORD(w_param)), ComboBox_GetCurSel(GetDlgItem(hwnd, LOWORD(w_param)))) && ComboBox_GetCurSel(GetDlgItem(hwnd, LOWORD(w_param))) != CB_ERR;

                switch (LOWORD(w_param))
                {
                case IDC_COMBO_GFX:
                    EnableWindow(GetDlgItem(hwnd, IDM_VIDEO_SETTINGS), has_plugin_selected);
                    EnableWindow(GetDlgItem(hwnd, IDGFXTEST), has_plugin_selected);
                    EnableWindow(GetDlgItem(hwnd, IDGFXABOUT), has_plugin_selected);
                    break;
                case IDC_COMBO_SOUND:
                    EnableWindow(GetDlgItem(hwnd, IDM_AUDIO_SETTINGS), has_plugin_selected);
                    EnableWindow(GetDlgItem(hwnd, IDSOUNDTEST), has_plugin_selected);
                    EnableWindow(GetDlgItem(hwnd, IDSOUNDABOUT), has_plugin_selected);
                    break;
                case IDC_COMBO_INPUT:
                    EnableWindow(GetDlgItem(hwnd, IDM_INPUT_SETTINGS), has_plugin_selected);
                    EnableWindow(GetDlgItem(hwnd, IDINPUTTEST), has_plugin_selected);
                    EnableWindow(GetDlgItem(hwnd, IDINPUTABOUT), has_plugin_selected);
                    break;
                case IDC_COMBO_RSP:
                    EnableWindow(GetDlgItem(hwnd, IDM_RSP_SETTINGS), has_plugin_selected);
                    EnableWindow(GetDlgItem(hwnd, IDRSPTEST), has_plugin_selected);
                    EnableWindow(GetDlgItem(hwnd, IDRSPABOUT), has_plugin_selected);
                    break;
                default:
                    break;
                }
                break;
            }
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
    size_t id = 1;

    t_options_group interface_group = {.id = ++id, .name = L"Interface"};

    t_options_group statusbar_group = {.id = ++id, .name = L"Statusbar"};

    t_options_group seek_piano_roll_group = {.id = ++id, .name = L"Seek / Piano Roll"};

    t_options_group flow_group = {.id = ++id, .name = L"Flow"};

    t_options_group capture_group = {.id = ++id, .name = L"Capture"};

    t_options_group core_group = {.id = ++id, .name = L"Core"};

    t_options_group lua_group = {.id = ++id, .name = L"Lua"};

    t_options_group debug_group = {.id = ++id, .name = L"Debug"};

    t_options_group hotkey_group = {.id = ++id, .name = L"Hotkeys"};

    groups = {interface_group, statusbar_group, seek_piano_roll_group, flow_group, capture_group, core_group, lua_group, debug_group, hotkey_group};

    options = {
    t_options_item{
    .group_id = interface_group.id,
    .name = L"Pause when unfocused",
    .tooltip = L"Pause emulation when the main window isn't in focus.",
    .data = &g_config.is_unfocused_pause_enabled,
    .type = t_options_item::Type::Bool,
    },
    t_options_item{
    .group_id = interface_group.id,
    .name = L"Silent Mode",
    .tooltip = L"Suppresses all dialogs and chooses reasonable defaults for "
               L"multiple-choice dialogs.\nCan cause data loss during normal "
               L"usage; only enable in automation scenarios!",
    .data = &g_config.silent_mode,
    .type = t_options_item::Type::Bool,
    },
    t_options_item{
    .group_id = interface_group.id,
    .name = L"Keep working directory",
    .tooltip = L"Keep the working directory specified by the caller "
               L"program at startup.\nWhen disabled, mupen changes the "
               L"working directory to its current path.",
    .data = &g_config.keep_default_working_directory,
    .type = t_options_item::Type::Bool,
    },
    t_options_item{
    .group_id = interface_group.id,
    .name = L"Use Async Executor",
    .tooltip = L"Whether the new async executor is used for async "
               L"calls.\nLowers interaction latency in general usecases.\nOn - "
               L"Each call is put on the async execution queue\nOff - Each call "
               L"runs on a newly created thread (legacy behaviour)",
    .data = &g_config.use_async_executor,
    .type = t_options_item::Type::Bool,
    },
    t_options_item{
    .group_id = interface_group.id,
    .name = L"Async Plugin Discovery",
    .tooltip = L"Whether plugins discovery is performed asynchronously. "
               L"Removes potential waiting times in the config dialog.",
    .data = &g_config.plugin_discovery_async,
    .type = t_options_item::Type::Bool,
    },

    t_options_item{
    .group_id = statusbar_group.id,
    .name = L"Layout",
    .tooltip = L"The statusbar layout preset.\nClassic - The legacy "
               L"layout\nModern - The new layout containing additional "
               L"information\nModern+ - The new layout, but with a "
               L"section for read-only status",
    .data = &g_config.statusbar_layout,
    .type = t_options_item::Type::Enum,
    .possible_values =
    {
    std::make_pair(L"Classic", (int32_t)StatusbarLayout::Classic),
    std::make_pair(L"Modern", (int32_t)StatusbarLayout::Modern),
    std::make_pair(L"Modern+", (int32_t)StatusbarLayout::ModernWithReadonly),
    },
    },
    t_options_item{
    .group_id = statusbar_group.id,
    .name = L"Zero-index",
    .tooltip = L"Show indicies in the statusbar, such as VCR frame "
               L"counts, relative to 0 instead of 1.",
    .data = &g_config.vcr_0_index,
    .type = t_options_item::Type::Bool,
    },
    t_options_item{
    .group_id = statusbar_group.id,
    .name = L"Scale down to fit window",
    .tooltip = L"Whether the statusbar is allowed to scale its segments down.",
    .data = &g_config.statusbar_scale_down,
    .type = t_options_item::Type::Bool,
    },
    t_options_item{
    .group_id = statusbar_group.id,
    .name = L"Scale up to fill window",
    .tooltip = L"Whether the statusbar is allowed to scale its segments up.",
    .data = &g_config.statusbar_scale_up,
    .type = t_options_item::Type::Bool,
    },

    t_options_item{
    .group_id = seek_piano_roll_group.id,
    .name = L"Savestate Interval",
    .tooltip = L"The interval at which to create savestates for seeking. Piano "
               L"Roll is exclusively read-only if this value is 0.\nHigher "
               L"numbers will reduce the seek duration at cost of emulator "
               L"performance, a value of 1 is not allowed.\n0 - Seek savestate "
               L"generation disabled\nRecommended: 100",
    .data = &g_config.seek_savestate_interval,
    .type = t_options_item::Type::Number,
    .is_readonly = [] { return VCR::get_task() != e_task::idle; },
    },
    t_options_item{
    .group_id = seek_piano_roll_group.id,
    .name = L"Savestate Max Count",
    .tooltip = L"The maximum amount of savestates to keep in memory for "
               L"seeking.\nHigher numbers might cause an out of memory "
               L"exception.",
    .data = &g_config.seek_savestate_max_count,
    .type = t_options_item::Type::Number,
    },
    t_options_item{
    .group_id = seek_piano_roll_group.id,
    .name = L"Constrain edit to column",
    .tooltip = L"Whether piano roll edits are constrained to the column "
               L"they started on.",
    .data = &g_config.piano_roll_constrain_edit_to_column,
    .type = t_options_item::Type::Bool,
    },
    t_options_item{
    .group_id = seek_piano_roll_group.id,
    .name = L"History size",
    .tooltip = L"Maximum size of the history list.",
    .data = &g_config.piano_roll_undo_stack_size,
    .type = t_options_item::Type::Number,
    },
    t_options_item{
    .group_id = seek_piano_roll_group.id,
    .name = L"Keep selection visible",
    .tooltip = L"Whether the piano roll will try to keep the selection visible.",
    .data = &g_config.piano_roll_keep_selection_visible,
    .type = t_options_item::Type::Bool,
    },
    t_options_item{
    .group_id = seek_piano_roll_group.id,
    .name = L"Keep playhead visible",
    .tooltip = L"Whether the piano roll will try to keep the playhead visible.",
    .data = &g_config.piano_roll_keep_playhead_visible,
    .type = t_options_item::Type::Bool,
    },

    t_options_item{
    .group_id = capture_group.id,
    .name = L"Delay",
    .tooltip = L"Miliseconds to wait before capturing a frame. Useful "
               L"for syncing with external programs.",
    .data = &g_config.capture_delay,
    .type = t_options_item::Type::Number,
    },
    t_options_item{
    .group_id = capture_group.id,
    .name = L"Encoder",
    .tooltip = L"The encoder to use when generating an output file.\nVFW - Slow "
               L"but stable (recommended)\nFFmpeg - Fast but less stable",
    .data = &g_config.encoder_type,
    .type = t_options_item::Type::Enum,
    .possible_values =
    {
    std::make_pair(L"VFW", (int32_t)EncoderType::VFW),
    std::make_pair(L"FFmpeg (experimental)", (int32_t)EncoderType::FFmpeg),
    },
    .is_readonly = [] { return EncodingManager::is_capturing(); },
    },
    t_options_item{
    .group_id = capture_group.id,
    .name = L"Mode",
    .tooltip = L"The video source to use for capturing video frames.\nPlugin - "
               L"Captures frames solely from the video plugin\nWindow - "
               L"Captures frames from the main window\nScreen - Captures "
               L"screenshots of the current display and crops them to "
               L"Mupen\nHybrid - Combines video plugin capture and internal Lua "
               L"composition (recommended)",
    .data = &g_config.capture_mode,
    .type = t_options_item::Type::Enum,
    .possible_values =
    {
    std::make_pair(L"Plugin", 0),
    std::make_pair(L"Window", 1),
    std::make_pair(L"Screen", 2),
    std::make_pair(L"Hybrid", 3),
    },
    .is_readonly = [] { return EncodingManager::is_capturing(); },
    },
    t_options_item{
    .group_id = capture_group.id,
    .name = L"Sync",
    .tooltip = L"The strategy to use for synchronizing video and audio during "
               L"capture.\nNone - No synchronization\nAudio - Audio is "
               L"synchronized to video\nVideo - Video is synchronized to audio",
    .data = &g_config.synchronization_mode,
    .type = t_options_item::Type::Enum,
    .possible_values =
    {
    std::make_pair(L"None", 0),
    std::make_pair(L"Audio", 1),
    std::make_pair(L"Video", 2),
    },
    .is_readonly = [] { return EncodingManager::is_capturing(); },
    },
    t_options_item{
    .group_id = capture_group.id,
    .name = L"FFmpeg Path",
    .tooltip = L"The path to the FFmpeg executable to use for capturing.",
    .data_str = &g_config.ffmpeg_path,
    .type = t_options_item::Type::String,
    .is_readonly = [] { return EncodingManager::is_capturing(); },
    },
    t_options_item{
    .group_id = capture_group.id,
    .name = L"FFmpeg Arguments",
    .tooltip = L"The argument format string to be passed to FFmpeg when "
               L"capturing.",
    .data_str = &g_config.ffmpeg_final_options,
    .type = t_options_item::Type::String,
    .is_readonly = [] { return EncodingManager::is_capturing(); },
    },

    t_options_item{
    .group_id = core_group.id,
    .name = L"Type",
    .tooltip = L"The core type to utilize for emulation.\nInterpreter - "
               L"Slow and relatively accurate\nDynamic Recompiler - "
               L"Fast, possibly less accurate, and only for x86 "
               L"processors\nPure Interpreter - Very slow and accurate",
    .data = &g_config.core_type,
    .type = t_options_item::Type::Enum,
    .possible_values =
    {
    std::make_pair(L"Interpreter", 0),
    std::make_pair(L"Dynamic Recompiler", 1),
    std::make_pair(L"Pure Interpreter", 2),
    },
    .is_readonly = [] { return emu_launched; },
    },
    t_options_item{
    .group_id = core_group.id,
    .name = L"Undo Savestate Load",
    .tooltip = L"Whether undo savestate load functionality is enabled.",
    .data = &g_config.st_undo_load,
    .type = t_options_item::Type::Bool,
    },
    t_options_item{
    .group_id = core_group.id,
    .name = L"Counter Factor",
    .tooltip = L"The CPU's counter factor.\nValues above 1 are "
               L"effectively 'lagless'.",
    .data = &g_config.counter_factor,
    .type = t_options_item::Type::Number,
    },
    t_options_item{
    .group_id = core_group.id,
    .name = L"Max Lag Frames",
    .tooltip = L"The maximum amount of lag frames before the core emits "
               L"a warning\n0 - Disabled",
    .data = &g_config.max_lag,
    .type = t_options_item::Type::Number,
    },
    t_options_item{
    .group_id = core_group.id,
    .name = L"Audio Delay",
    .tooltip = L"Whether to delay audio interrupts\nEnabled - More stability",
    .data = &g_config.is_audio_delay_enabled,
    .type = t_options_item::Type::Bool,
    },
    t_options_item{
    .group_id = core_group.id,
    .name = L"Compiled Jump",
    .tooltip = L"Whether to compile jumps\nEnabled - More stability",
    .data = &g_config.is_compiled_jump_enabled,
    .type = t_options_item::Type::Bool,
    },
    t_options_item{
    .group_id = core_group.id,
    .name = L"WiiVC Mode",
    .tooltip = L"Enables WiiVC emulation.",
    .data = &g_config.wii_vc_emulation,
    .type = t_options_item::Type::Bool,
    },
    t_options_item{
    .group_id = core_group.id,
    .name = L"Auto-increment Slot",
    .tooltip = L"Automatically increment the save slot upon saving a state.",
    .data = &g_config.increment_slot,
    .type = t_options_item::Type::Bool,
    },
    t_options_item{
    .group_id = core_group.id,
    .name = L"Emulate Float Crashes",
    .tooltip = L"Emulate float operation-related crashes which would "
               L"also crash on real hardware",
    .data = &g_config.is_float_exception_propagation_enabled,
    .type = t_options_item::Type::Bool,
    },
    t_options_item{
    .group_id = core_group.id,
    .name = L"Movie Backups",
    .tooltip = L"Generate a backup of the currently recorded movie when loading "
               L"a savestate.\nBackups are saved in the backups folder.",
    .data = &g_config.vcr_backups,
    .type = t_options_item::Type::Bool,
    },
    t_options_item{
    .group_id = core_group.id,
    .name = L"Fast-Forward Skip Frequency",
    .tooltip = L"Skip rendering every nth frame when in fast-forward "
               L"mode.\n0 - Render nothing\n1 - Render every frame\nn - "
               L"Render every nth frame",
    .data = &g_config.frame_skip_frequency,
    .type = t_options_item::Type::Number,
    },
    t_options_item{
    .group_id = core_group.id,
    .name = L"Record Resets",
    .tooltip = L"Record manually performed resets to the current movie.\nThese "
               L"resets will be repeated when the movie is played back.",
    .data = &g_config.is_reset_recording_enabled,
    .type = t_options_item::Type::Bool,
    },
    t_options_item{
    .group_id = core_group.id,
    .name = L"Emulate SD Card",
    .tooltip = L"Enable SD card emulation.\nRequires a VHD-formatted SD "
               L"card file named card.vhd in the same folder as Mupen.",
    .data = &g_config.is_reset_recording_enabled,
    .type = t_options_item::Type::Bool,
    },
    t_options_item{
    .group_id = core_group.id,
    .name = L"Instant Savestate Update",
    .tooltip = L"Saves and loads game graphics to savestates to allow instant "
               L"graphics updates when loading savestates.\nGreatly increases "
               L"savestate saving and loading time.",
    .data = &g_config.st_screenshot,
    .type = t_options_item::Type::Bool,
    },
    t_options_item{
    .group_id = core_group.id,
    .name = L"Skip rendering lag",
    .tooltip = L"Prevents calls to updateScreen during lag.\nMight improve "
               L"performance on some video plugins at the cost of stability.",
    .data = &g_config.skip_rendering_lag,
    .type = t_options_item::Type::Bool,
    },
    t_options_item{
    .group_id = core_group.id,
    .name = L"ROM Cache Size",
    .tooltip = L"Size of the ROM cache.\nImproves ROM loading performance at "
               L"the cost of data staleness and high memory usage.\n0 - "
               L"Disabled\nn - Maximum of n ROMs kept in cache",
    .data = &g_config.rom_cache_size,
    .type = t_options_item::Type::Number,
    },

    t_options_item{
    .group_id = lua_group.id,
    .name = L"Presenter",
    .tooltip = L"The presenter type to use for displaying and capturing "
               L"Lua graphics.\nRecommended: DirectComposition",
    .data = &g_config.presenter_type,
    .type = t_options_item::Type::Enum,
    .possible_values =
    {
    std::make_pair(L"DirectComposition", (int32_t)PresenterType::DirectComposition),
    std::make_pair(L"GDI", (int32_t)PresenterType::GDI),
    },
    .is_readonly = [] { return !g_hwnd_lua_map.empty(); },
    },

    t_options_item{
    .group_id = debug_group.id,
    .name = L"Async Executor Cuzz",
    .tooltip = L"Whether the async executor will apply concurrency fuzzing. "
               L"When enabled, task execution will be delayed to expose delayed "
               L"task execution handling deficiencies at the callsite.\nDo not "
               L"enable unless you are debugging the async executor.",
    .data = &g_config.async_executor_cuzz,
    .type = t_options_item::Type::Bool,
    },
    };

    for (const auto hotkey : g_config_hotkeys)
    {
        options.push_back(t_options_item{
        .group_id = hotkey_group.id,
        .name = hotkey->identifier,
        .tooltip = std::format(L"{} hotkey.\nAction down: #{}\nAction up: #{}", hotkey->identifier, (int32_t)hotkey->down_cmd, (int32_t)hotkey->up_cmd),
        .data = reinterpret_cast<int32_t*>(hotkey),
        .type = t_options_item::Type::Hotkey,
        });
    }
}

LRESULT CALLBACK InlineEditBoxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId, DWORD_PTR dwRefData)
{
    switch (msg)
    {
    case WM_GETDLGCODE:
        {
            if (wParam == VK_RETURN)
            {
                goto apply;
            }
            if (wParam == VK_ESCAPE)
            {
                DestroyWindow(hwnd);
            }
            break;
        }
    case WM_KILLFOCUS:
        goto apply;
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, InlineEditBoxProc, sId);
        g_edit_hwnd = nullptr;
        break;
    default:
        break;
    }

def:
    return DefSubclassProc(hwnd, msg, wParam, lParam);

apply:

    const auto len = Edit_GetTextLength(hwnd) + 1;

    if (len <= 0)
    {
        goto def;
    }

    auto str = static_cast<wchar_t*>(calloc(len, sizeof(wchar_t)));

    if (!str)
    {
        goto def;
    }

    Edit_GetText(hwnd, str, len);

    SendMessage(GetParent(hwnd), WM_EDIT_END, 0, (LPARAM)str);

    free(str);

    DestroyWindow(hwnd);

    goto def;
}

INT_PTR CALLBACK EditStringDialogProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        {
            auto option_item = g_option_items[g_edit_option_item_index];
            auto edit_hwnd = GetDlgItem(wnd, IDC_TEXTBOX_LUAPROMPT);

            SetWindowText(wnd, std::format(L"Edit '{}'", option_item.name).c_str());
            Edit_SetText(edit_hwnd, option_item.data_str->data());

            SetFocus(GetDlgItem(wnd, IDC_TEXTBOX_LUAPROMPT));
            break;
        }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            {
                auto edit_hwnd = GetDlgItem(wnd, IDC_TEXTBOX_LUAPROMPT);

                auto len = Edit_GetTextLength(edit_hwnd) + 1;
                auto str = static_cast<wchar_t*>(calloc(len, sizeof(wchar_t)));
                Edit_GetText(edit_hwnd, str, len);

                *g_option_items[g_edit_option_item_index].data_str = str;

                free(str);

                EndDialog(wnd, 0);
                break;
            }
        case IDCANCEL:
            EndDialog(wnd, 1);
            break;
        }
        break;
    }
    return FALSE;
}

/**
 * Advances the settings listview's selection by one.
 */
void advance_listview_selection()
{
    int32_t i = ListView_GetNextItem(g_lv_hwnd, -1, LVNI_SELECTED);
    if (i == -1)
        return;
    ListView_SetItemState(g_lv_hwnd, i, 0, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_SetItemState(g_lv_hwnd, i + 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(g_lv_hwnd, i + 1, false);
}

bool begin_listview_edit(HWND hwnd)
{
    int32_t i = ListView_GetNextItem(g_lv_hwnd, -1, LVNI_SELECTED);

    if (i == -1)
        return false;

    LVITEM item = {0};
    item.mask = LVIF_PARAM;
    item.iItem = i;
    ListView_GetItem(g_lv_hwnd, &item);

    auto option_item = g_option_items[item.lParam];

    // TODO: Perhaps gray out readonly values too?
    if (option_item.is_readonly())
    {
        return false;
    }

    // For bools, just flip the value...
    if (option_item.type == OptionsItem::Type::Bool)
    {
        *option_item.data ^= true;
    }

    // For enums, cycle through the possible values
    if (option_item.type == OptionsItem::Type::Enum)
    {
        // 1. Find the index of the currently selected item, while falling back to
        // the first possible value if there's no match
        size_t current_value = option_item.possible_values[0].second;
        for (const auto& [_, possible_value] : option_item.possible_values)
        {
            if (*option_item.data == possible_value)
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
            if (val > max_possible_value)
            {
                max_possible_value = val;
            }
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
        *option_item.data = current_value;
    }

    // For strings, allow editing in a dialog (since it might be a multiline
    // string and we can't really handle that below)
    if (option_item.type == OptionsItem::Type::String)
    {
        g_edit_option_item_index = item.lParam;
        DialogBoxParam(g_app_instance, MAKEINTRESOURCE(IDD_LUAINPUTPROMPT), hwnd, EditStringDialogProc, 0);
    }

    // For numbers, create a textbox over the value cell for inline editing
    if (option_item.type == OptionsItem::Type::Number)
    {
        if (g_edit_hwnd)
        {
            DestroyWindow(g_edit_hwnd);
        }

        g_edit_option_item_index = item.lParam;

        RECT item_rect{};
        ListView_GetSubItemRect(g_lv_hwnd, i, 1, LVIR_LABEL, &item_rect);

        RECT lv_rect{};
        GetClientRect(g_lv_hwnd, &lv_rect);

        item_rect.right = lv_rect.right;

        g_edit_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP, item_rect.left, item_rect.top, item_rect.right - item_rect.left, item_rect.bottom - item_rect.top, hwnd, 0, g_app_instance, 0);

        SendMessage(g_edit_hwnd, WM_SETFONT, (WPARAM)SendMessage(g_lv_hwnd, WM_GETFONT, 0, 0), 0);

        SetWindowSubclass(g_edit_hwnd, InlineEditBoxProc, 0, 0);

        Edit_SetText(g_edit_hwnd, std::to_wstring(*option_item.data).c_str());

        PostMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)g_edit_hwnd, TRUE);
    }

    // For hotkeys, accept keyboard inputs
    if (option_item.type == OptionsItem::Type::Hotkey)
    {
        t_hotkey* hotkey = (t_hotkey*)option_item.data;
        g_hotkey_active_index = std::make_optional(i);
        ListView_Update(g_lv_hwnd, i);
        RedrawWindow(g_lv_hwnd, nullptr, nullptr, RDW_UPDATENOW);
        get_user_hotkey(hotkey);
        g_hotkey_active_index.reset();

        advance_listview_selection();
    }

    ListView_Update(g_lv_hwnd, i);
    return true;
}

LRESULT CALLBACK list_view_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param, UINT_PTR, DWORD_PTR)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        if (w_param == VK_SPACE)
        {
            return TRUE;
        }
        break;
    case WM_KEYUP:
        if (w_param == VK_SPACE && begin_listview_edit(hwnd))
        {
            return TRUE;
        }
        break;
    default:
        break;
    }
    return DefSubclassProc(hwnd, msg, w_param, l_param);
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

            g_lv_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL, WS_TABSTOP | WS_VISIBLE | WS_CHILD | LVS_SINGLESEL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNTOP, grid_rect.left, grid_rect.top, grid_rect.right - grid_rect.left, grid_rect.bottom - grid_rect.top, hwnd, (HMENU)IDC_SETTINGS_LV, g_app_instance, NULL);
            SetWindowSubclass(g_lv_hwnd, list_view_proc, 0, 0);

            HIMAGELIST image_list = ImageList_Create(16, 16, ILC_COLORDDB | ILC_MASK, 2, 0);
            ImageList_AddIcon(image_list, LoadIcon(g_app_instance, MAKEINTRESOURCE(IDI_DENY)));
            ImageList_AddIcon(image_list, LoadIcon(g_app_instance, MAKEINTRESOURCE(IDI_CHANGED)));
            ListView_SetImageList(g_lv_hwnd, image_list, LVSIL_SMALL);

            ListView_EnableGroupView(g_lv_hwnd, true);
            ListView_SetExtendedListViewStyle(g_lv_hwnd, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_LABELTIP);

            for (auto group : g_option_groups)
            {
                LVGROUP lvgroup;
                lvgroup.cbSize = sizeof(LVGROUP);
                lvgroup.mask = LVGF_HEADER | LVGF_GROUPID;
                // FIXME: This is concerning, but seems to work
                lvgroup.pszHeader = const_cast<wchar_t*>(group.name.c_str());
                lvgroup.iGroupId = group.id;
                ListView_InsertGroup(g_lv_hwnd, -1, &lvgroup);
            }

            LVCOLUMN lv_column = {0};
            lv_column.mask = LVCF_FMT | LVCF_DEFAULTWIDTH | LVCF_TEXT | LVCF_SUBITEM;

            lv_column.pszText = const_cast<LPWSTR>(L"Name");
            ListView_InsertColumn(g_lv_hwnd, 0, &lv_column);
            lv_column.pszText = const_cast<LPWSTR>(L"Value");
            ListView_InsertColumn(g_lv_hwnd, 1, &lv_column);

            LVITEM lv_item = {0};
            lv_item.mask = LVIF_TEXT | LVIF_GROUPID | LVIF_PARAM | LVIF_IMAGE;
            lv_item.pszText = LPSTR_TEXTCALLBACK;
            lv_item.iImage = I_IMAGECALLBACK;
            for (int i = 0; i < g_option_items.size(); ++i)
            {
                lv_item.iGroupId = g_option_items[i].group_id;
                lv_item.lParam = i;
                lv_item.iItem = i;
                ListView_InsertItem(g_lv_hwnd, &lv_item);
            }

            ListView_SetColumnWidth(g_lv_hwnd, 0, LVSCW_AUTOSIZE_USEHEADER);
            ListView_SetColumnWidth(g_lv_hwnd, 1, LVSCW_AUTOSIZE_USEHEADER);
            ListView_SetColumnWidth(g_lv_hwnd, 2, LVSCW_AUTOSIZE_USEHEADER);

            return TRUE;
        }
    case WM_EDIT_END:
        {
            auto str = reinterpret_cast<wchar_t*>(l_param);

            if (g_option_items[g_edit_option_item_index].type == OptionsItem::Type::Number)
            {
                try
                {
                    auto result = std::stoi(str);
                    *g_option_items[g_edit_option_item_index].data = result;
                }
                catch (...)
                {
                    // ignored
                }
            }
            else
            {
                *g_option_items[g_edit_option_item_index].data_str = str;
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
            if (option_item.type == OptionsItem::Type::Hotkey)
            {
                AppendMenu(h_menu, MF_SEPARATOR, 3, L"");
                AppendMenu(h_menu, MF_STRING, 4, L"Clear");
            }
            AppendMenu(h_menu, MF_SEPARATOR, 100, L"");
            AppendMenu(h_menu, MF_STRING, 5, L"Reset all to default");

            const int offset = TrackPopupMenuEx(h_menu, TPM_RETURNCMD | TPM_NONOTIFY, GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param), hwnd, 0);

            if (offset < 0)
            {
                break;
            }

            void* default_equivalent = option_item.get_default_value_ptr(&g_default_config);

            if (offset == 1)
            {
                option_item.reset_to_default();

                ListView_Update(g_lv_hwnd, i);
            }

            if (offset == 2)
            {
                // Show a MessageBox with the tooltip and all possible values along with
                // the default value.

                std::wstring str;

                str += option_item.tooltip;

                if (!option_item.possible_values.empty())
                {
                    str += L"\r\n\r\n";

                    for (auto pair : option_item.possible_values)
                    {
                        str += std::format(L"{} - {}", pair.second, pair.first);

                        if (pair.second == *(int32_t*)default_equivalent)
                        {
                            str += L" (default)";
                        }

                        str += L"\r\n";
                    }
                }

                FrontendService::show_dialog(str.c_str(), L"Information", FrontendService::DialogType::Information, hwnd);
            }

            if (offset == 4 && option_item.type == OptionsItem::Type::Hotkey)
            {
                auto current_hotkey = (t_hotkey*)option_item.data;
                current_hotkey->key = 0;
                current_hotkey->ctrl = 0;
                current_hotkey->alt = 0;
                current_hotkey->shift = 0;
                ListView_Update(g_lv_hwnd, i);
            }

            if (offset == 5)
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
                    FrontendService::show_dialog(L"Some settings can't be reset, as they are currently read-only. "
                                                 L"Try again with emulation stopped.\nNo changes have been made to "
                                                 L"the settings.",
                                                 L"Reset all to default", FrontendService::DialogType::Warning, hwnd);
                    goto destroy_menu;
                }

                const auto result = FrontendService::show_ask_dialog(L"Are you sure you want to reset all settings to default?", L"Reset all to default", false, hwnd);

                if (!result)
                {
                    goto destroy_menu;
                }

                for (auto& v : g_option_items)
                {
                    v.reset_to_default();
                }
                ListView_RedrawItems(g_lv_hwnd, 0, ListView_GetItemCount(g_lv_hwnd));
            }

        destroy_menu:
            DestroyMenu(h_menu);
        }
        break;
    case WM_NOTIFY:
        {
            if (lpnmhdr->code == PSN_SETACTIVE)
            {
                g_config.settings_tab = 2;
            }

            if (w_param == IDC_SETTINGS_LV)
            {
                switch (lpnmhdr->code)
                {
                case LVN_GETDISPINFO:
                    {
                        const auto plvdi = reinterpret_cast<NMLVDISPINFOW*>(l_param);
                        t_options_item options_item = g_option_items[plvdi->item.lParam];

                        if (plvdi->item.mask & LVIF_IMAGE)
                        {
                            void* default_value_ptr = options_item.get_default_value_ptr(&g_prev_config);

                            if (options_item.type == OptionsItem::Type::String)
                            {
                                plvdi->item.iImage = *(std::wstring*)default_value_ptr == *options_item.data_str ? 50 : 1;
                            }
                            else if (options_item.type == OptionsItem::Type::Hotkey)
                            {
                                auto default_hotkey = (t_hotkey*)default_value_ptr;
                                auto current_hotkey = (t_hotkey*)options_item.data;

                                bool same = default_hotkey->key == current_hotkey->key && default_hotkey->ctrl == current_hotkey->ctrl && default_hotkey->alt == current_hotkey->alt && default_hotkey->shift == current_hotkey->shift;

                                plvdi->item.iImage = same ? 50 : 1;
                            }
                            else
                            {
                                plvdi->item.iImage = *(int32_t*)default_value_ptr == *options_item.data ? 50 : 1;
                            }

                            if (options_item.is_readonly())
                            {
                                plvdi->item.iImage = 0;
                            }
                        }

                        switch (plvdi->item.iSubItem)
                        {
                        case 0:
                            StrNCpy(plvdi->item.pszText, options_item.name.c_str(), plvdi->item.cchTextMax);
                            break;
                        case 1:
                            if (g_hotkey_active_index.has_value() && g_hotkey_active_index.value() == plvdi->item.iItem)
                            {
                                StrNCpy(plvdi->item.pszText, L"... (RMB to cancel)", plvdi->item.cchTextMax);
                                break;
                            }
                            StrNCpy(plvdi->item.pszText, options_item.get_value_name().c_str(), plvdi->item.cchTextMax);
                            break;
                        }
                        break;
                    }
                case LVN_GETINFOTIP:
                    {
                        auto getinfotip = (LPNMLVGETINFOTIP)l_param;

                        LVITEM item = {0};
                        item.mask = LVIF_PARAM;
                        item.iItem = getinfotip->iItem;
                        ListView_GetItem(g_lv_hwnd, &item);

                        auto option_item = g_option_items[item.lParam];

                        if (option_item.tooltip.empty())
                        {
                            break;
                        }

                        StrCpyNW(getinfotip->pszText, option_item.tooltip.c_str(), getinfotip->cchTextMax);

                        break;
                    }
                case NM_DBLCLK:
                    if (begin_listview_edit(hwnd))
                    {
                        return TRUE;
                    }
                    break;
                default:
                    break;
                }
            }
            break;
        }
    case WM_DESTROY:
        {
            for (auto tooltip : tooltips)
            {
                DestroyWindow(tooltip);
            }
            tooltips.clear();
            break;
        }
    default:
        return FALSE;
    }
    return TRUE;
}

void configdialog_init() { get_config_listview_items(g_option_groups, g_option_items); }

void configdialog_show()
{
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

    if (!PropertySheet(&psh))
    {
        g_config = g_prev_config;
    }

    save_config();
    Messenger::broadcast(Messenger::Message::ConfigLoaded, nullptr);
}
