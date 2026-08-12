/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Common.Views/ActionManager.hpp>
#include <Common.Views/Config.hpp>
#include <Common.Views/IDialogService.hpp>
#include <Common.Views/Messages.hpp>
#include <plugin/Plugin.hpp>
#include <CaptureManager.hpp>
#include <components/FilePicker.hpp>
#include <SettingsListView.hpp>
#include <components/TextEditDialog.hpp>
#include <components/ConfigDialog.hpp>
#include <lua/LuaManager.hpp>
#include <Common.Views/Hotkey.hpp>
#include <HotkeyUtils.hpp>

#define WM_EDIT_END (WM_USER + 19)
#define WM_PLUGIN_DISCOVERY_FINISHED (WM_USER + 22)

using t_options_group = ConfigDialog::t_options_group;
using t_options_item = ConfigDialog::t_options_item;

t_plugin_discovery_result plugin_discovery_result;
std::vector<t_options_group> g_option_groups;
std::vector<t_options_item> g_option_items;
static std::vector<t_options_group> g_static_option_groups;
t_config g_prev_config;

std::thread g_plugin_discovery_thread;

// Whether a plugin rescan is needed. Set when modifying the plugin path.
bool g_plugin_discovery_rescan = false;

struct t_tab_context
{
    size_t tab_index;

    // The groups to show in this tab.
    std::vector<std::string> groups;

    HWND hwnd;
    HWND lv_hwnd;
    HWND edit_hwnd;
    size_t edit_option_item_index;
    std::unordered_map<size_t, size_t> item_index_map;
};

static std::string to_str_default(const double value)
{
    return std::format("{:.15g}", value);
}

static double get_number_value(const t_options_item::data_variant &value)
{
    if (std::holds_alternative<int32_t>(value))
    {
        return static_cast<double>(std::get<int32_t>(value));
    }
    if (std::holds_alternative<double>(value))
    {
        return std::get<double>(value);
    }

    RT_ASSERT(false, "Number option does not hold an int32_t or double value");
    return 0.0;
}

static t_options_item::data_variant parse_number_value(const std::string &text,
                                                       const t_options_item::data_variant &current)
{
    if (std::holds_alternative<int32_t>(current))
    {
        return std::stoi(text);
    }
    if (std::holds_alternative<double>(current))
    {
        return std::stod(text);
    }

    RT_ASSERT(false, "Number option does not hold an int32_t or double value");
    return current;
}

std::string t_options_item::get_name() const
{
    if (type == Type::Hotkey) return ActionManager::get_display_name(name, true);
    return name;
}

std::string t_options_item::get_value_name() const
{
    const auto value = current_value.get();

    switch (type)
    {
    case Type::Bool:
        return std::get<int32_t>(value) != 0 ? "On" : "Off";
    case Type::Number:
        return to_str_default(get_number_value(value));
    case Type::Enum: {
        const auto enum_value = std::get<int32_t>(value);

        for (const auto &pair : possible_values)
        {
            if (enum_value == pair.second)
            {
                return pair.first;
            }
        }

        return std::format("Unknown ({})", enum_value);
    }
    case Type::String:
        return std::get<std::string>(value);
    case Type::Hotkey:
        return std::get<Hotkey>(value).to_string();
    case Type::Folder:
        return std::get<std::string>(value);
    default:
        RT_ASSERT(false, "Unhandled option type in t_options_item::get_value_name");
    }
    return "";
}

void t_options_item::reset_to_default() const
{
    current_value.set(default_value.get());
}

std::string t_options_item::get_friendly_info() const
{
    std::string str = tooltip.empty() ? "(no further information available)" : tooltip;

    if (possible_values.empty())
    {
        return str;
    }

    str += "\r\n\r\n";
    for (const auto &pair : possible_values)
    {
        str += std::format("{} - {}", pair.second, pair.first);

        if (pair.second == std::get<int32_t>(current_value.get()))
        {
            str += " (default)";
        }

        str += "\r\n";
    }

    return str;
}

bool t_options_item::edit(const HWND hwnd)
{
    switch (type)
    {
    case Type::Bool: {
        const auto new_value = std::get<int32_t>(current_value.get()) == 0 ? 1 : 0;
        current_value.set(new_value);
        return true;
    }
    case Type::Number: {
        const auto value = current_value.get();
        const auto result = TextEditDialog::show({.parent_hwnd = hwnd,
                                                  .text = to_str_default(get_number_value(value)),
                                                  .caption = std::format("Edit value for {}", name)});
        if (!result.has_value())
        {
            break;
        }

        try
        {
            current_value.set(parse_number_value(result.value(), value));
            return true;
        }
        catch (...)
        {
        }
        break;
    }
    case Type::Enum: {
        // 1. Find the index of the currently selected item, while falling back to the first possible value if there's
        // no match
        int32_t val = possible_values[0].second;
        for (const auto &[_, possible_value] : possible_values)
        {
            if (std::get<int32_t>(current_value.get()) == possible_value)
            {
                val = possible_value;
                break;
            }
        }

        // 2. Find the lowest and highest values in the vector
        int32_t min_possible_value = INT32_MAX;
        int32_t max_possible_value = INT32_MIN;
        for (const auto &val : possible_values | std::views::values)
        {
            max_possible_value = std::max(val, max_possible_value);
            min_possible_value = std::min(val, min_possible_value);
        }

        // 2. Bump it, wrapping around if needed
        val++;
        if (val > max_possible_value)
        {
            val = min_possible_value;
        }

        // 3. Apply the change
        current_value.set(val);
        return true;
    }
    case Type::String: {
        const auto value = std::get<std::string>(current_value.get());
        const auto result = TextEditDialog::show(
            {.parent_hwnd = hwnd, .text = value, .caption = std::format("Edit value for {}", name)});
        if (result.has_value())
        {
            current_value.set(result.value());
            return true;
        }
        break;
    }
    case Type::Hotkey: {
        auto hotkey = std::get<Hotkey>(current_value.get());
        HotkeyUtils::show_prompt(hwnd, std::format("Choose a hotkey for {}", name), hotkey);
        HotkeyUtils::try_associate_hotkey(hwnd, name, hotkey, false);
        return true;
    }
    case Type::Folder: {
        const auto path = FilePicker::show_folder_dialog(this->name, hwnd);
        if (!path.empty())
        {
            current_value.set(path.string());
            return true;
        }
        break;
    }
    default:
        break;
    }

    return false;
}

INT_PTR CALLBACK plugin_discovery_dlgproc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    static HWND g_pldlv_hwnd;

    switch (msg)
    {
    case WM_INITDIALOG: {
        RECT rect{};
        GetClientRect(hwnd, &rect);

        g_pldlv_hwnd = CreateWindowEx(
            WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL,
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | LVS_SINGLESEL | LVS_REPORT | LVS_SHOWSELALWAYS, rect.left, rect.top,
            rect.right - rect.left, rect.bottom - rect.top, hwnd, nullptr, g_main_ctx.hinst, NULL);

        ListView_SetExtendedListViewStyle(g_pldlv_hwnd, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

        LVCOLUMN lv_column = {0};
        lv_column.mask = LVCF_FMT | LVCF_DEFAULTWIDTH | LVCF_TEXT | LVCF_SUBITEM;

        lv_column.pszText = const_cast<LPSTR>("Plugin");
        ListView_InsertColumn(g_pldlv_hwnd, 0, &lv_column);
        lv_column.pszText = const_cast<LPSTR>("Error");
        ListView_InsertColumn(g_pldlv_hwnd, 1, &lv_column);

        LV_ITEM lv_item = {0};
        lv_item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        lv_item.pszText = LPSTR_TEXTCALLBACK;

        size_t i = 0;
        for (const auto &pair : plugin_discovery_result.results)
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

        WinDarkMode::attach(hwnd);
        return TRUE;
    }
    case WM_NOTIFY: {
        switch (((LPNMHDR)l_param)->code)
        {
        case LVN_GETDISPINFO: {
            auto plvdi = reinterpret_cast<NMLVDISPINFO *>(l_param);
            const auto pair = plugin_discovery_result.results[plvdi->item.lParam];
            switch (plvdi->item.iSubItem)
            {
            case 0: {
                strncpy(plvdi->item.pszText, pair.first.filename().string().c_str(), plvdi->item.cchTextMax);
                break;
            }
            case 1: {
                strncpy(plvdi->item.pszText, pair.second.c_str(), plvdi->item.cchTextMax);
                break;
            }
            default:
                break;
            }
        }
        break;
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
    default:
        return FALSE;
    }
    return TRUE;
}

void update_plugin_selection(const HWND hwnd, const int32_t id, const std::filesystem::path &path)
{
    for (int i = 0; i < SendDlgItemMessage(hwnd, id, CB_GETCOUNT, 0, 0); ++i)
    {
        if (const auto plugin = (Plugin *)SendDlgItemMessage(hwnd, id, CB_GETITEMDATA, i, 0); plugin->path() == path)
        {
            ComboBox_SetCurSel(GetDlgItem(hwnd, id), i);
            break;
        }
    }
    SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(id, 0), 0);
}

Plugin *get_selected_plugin(const HWND hwnd, const int id)
{
    const int i = SendDlgItemMessage(hwnd, id, CB_GETCURSEL, 0, 0);
    const auto res = SendDlgItemMessage(hwnd, id, CB_GETITEMDATA, i, 0);
    return res == CB_ERR ? nullptr : (Plugin *)res;
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

    SetDlgItemText(hwnd, IDC_PLUGIN_WARNING, "Discovering plugins...");

    if (g_plugin_discovery_thread.joinable())
    {
        g_plugin_discovery_thread.join();
    }

    g_plugin_discovery_thread = std::thread([=] { start_plugin_discovery(hwnd); });
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

// Compares two configs, ignoring the settings_tab field because it's changed while in the settings dialog.
static bool weak_compare(const t_config &a, const t_config &b)
{
    t_config lhs = a;
    t_config rhs = b;

    lhs.settings_tab = 0;
    rhs.settings_tab = 0;

    lhs.core.total_frames = 0;
    rhs.core.total_frames = 0;

    lhs.core.total_rerecords = 0;
    rhs.core.total_rerecords = 0;

    return lhs == rhs;
}

static INT_PTR CALLBACK base_pageproc(const HWND hwnd, const UINT message, const WPARAM w_param, const LPARAM l_param)
{
    if (message == WM_NOTIFY)
    {
        const auto lpnmhdr = reinterpret_cast<LPNMHDR>(l_param);
        if (lpnmhdr->code == PSN_QUERYCANCEL)
        {
            if (!weak_compare(g_config, g_prev_config))
            {
                const auto result = g_dialog_service->show_ask_dialog(
                    VIEW_DLG_CONFIRM_SETTINGS_DISCARD,
                    "You have unsaved changes. Are you sure you want to discard the changes?", "Settings", true, hwnd);

                if (!result)
                {
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, 1);
                    return 1;
                }
            }
        }
    }
    return FALSE;
}

INT_PTR CALLBACK plugins_cfg(const HWND hwnd, const UINT message, const WPARAM w_param, const LPARAM l_param)
{
    const auto lpnmhdr = reinterpret_cast<LPNMHDR>(l_param);

    [[maybe_unused]] char path_buffer[_MAX_PATH];

    const auto base_result = base_pageproc(hwnd, message, w_param, l_param);
    if (base_result) return base_result;

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
    case WM_DRAWITEM: {
        const auto dis = reinterpret_cast<DRAWITEMSTRUCT *>(l_param);
        static constexpr std::array plugin_icon_ids = {IDB_DISPLAY, IDB_CONTROL, IDB_SOUND, IDB_RSP};
        if (dis->CtlType == ODT_STATIC && std::ranges::contains(plugin_icon_ids, static_cast<int>(dis->CtlID)))
        {
            draw_bitmap_transparent(dis->hDC, dis->rcItem, g_main_ctx.hinst, static_cast<int>(dis->CtlID),
                                    WinDarkMode::theme_data.bg_color == WinDarkMode::dark_theme_data.bg_color);
            return TRUE;
        }
        return FALSE;
    }
    case WM_INITDIALOG: {
        refresh_plugins_page(hwnd);

        WinDarkMode::attach(hwnd);
        return TRUE;
    }
    case WM_PLUGIN_DISCOVERY_FINISHED: {
        std::vector<std::pair<std::filesystem::path, std::string>> broken_plugins;

        std::ranges::copy_if(plugin_discovery_result.results, std::back_inserter(broken_plugins),
                             [](const auto &pair) { return !pair.second.empty(); });

        if (broken_plugins.empty())
        {
            SetDlgItemText(hwnd, IDC_PLUGIN_WARNING, "");
        }
        else
        {
            SetDlgItemText(
                hwnd, IDC_PLUGIN_WARNING,
                std::format("Not all discovered plugins shown. {} plugin(s) failed to load.", broken_plugins.size())
                    .c_str());
        }

        ShowWindow(GetDlgItem(hwnd, IDC_PLUGIN_DISCOVERY_INFO), !broken_plugins.empty() ? SW_SHOW : SW_HIDE);

        ComboBox_ResetContent(GetDlgItem(hwnd, IDC_COMBO_GFX));
        ComboBox_ResetContent(GetDlgItem(hwnd, IDC_COMBO_SOUND));
        ComboBox_ResetContent(GetDlgItem(hwnd, IDC_COMBO_INPUT));
        ComboBox_ResetContent(GetDlgItem(hwnd, IDC_COMBO_RSP));

        for (const auto &plugin : plugin_discovery_result.plugins)
        {
            int32_t id = 0;
            switch (plugin->type())
            {
            case Plugin::Type::Video:
                id = IDC_COMBO_GFX;
                break;
            case Plugin::Type::Audio:
                id = IDC_COMBO_SOUND;
                break;
            case Plugin::Type::Input:
                id = IDC_COMBO_INPUT;
                break;
            case Plugin::Type::RSP:
                id = IDC_COMBO_RSP;
                break;
            default:
                assert(false);
                break;
            }
            // we add the string and associate a pointer to the plugin with the item
            const int i = SendDlgItemMessage(hwnd, id, CB_GETCOUNT, 0, 0);
            SendDlgItemMessage(hwnd, id, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(plugin->name().c_str()));
            SendDlgItemMessage(hwnd, id, CB_SETITEMDATA, i, (LPARAM)plugin.get());
        }

        update_plugin_selection(hwnd, IDC_COMBO_GFX, g_config.selected_video_plugin);
        update_plugin_selection(hwnd, IDC_COMBO_SOUND, g_config.selected_audio_plugin);
        update_plugin_selection(hwnd, IDC_COMBO_INPUT, g_config.selected_input_plugin);
        update_plugin_selection(hwnd, IDC_COMBO_RSP, g_config.selected_rsp_plugin);

        const auto ids_to_enable = {
            IDM_VIDEO_SETTINGS, IDM_AUDIO_SETTINGS, IDM_INPUT_SETTINGS, IDM_RSP_SETTINGS, IDGFXTEST,    IDSOUNDTEST,
            IDINPUTTEST,        IDRSPTEST,          IDGFXABOUT,         IDSOUNDABOUT,     IDINPUTABOUT, IDRSPABOUT,
        };

        EnableWindow(GetDlgItem(hwnd, IDC_COMBO_GFX), !g_main_ctx.core_ctx->vr_get_launched());
        EnableWindow(GetDlgItem(hwnd, IDC_COMBO_INPUT), !g_main_ctx.core_ctx->vr_get_launched());
        EnableWindow(GetDlgItem(hwnd, IDC_COMBO_SOUND), !g_main_ctx.core_ctx->vr_get_launched());
        EnableWindow(GetDlgItem(hwnd, IDC_COMBO_RSP), !g_main_ctx.core_ctx->vr_get_launched());

        for (const auto &id : ids_to_enable)
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
            get_selected_plugin(hwnd, IDC_COMBO_GFX)->config(hwnd);
            break;
        case IDGFXTEST:
            get_selected_plugin(hwnd, IDC_COMBO_GFX)->test(hwnd);
            break;
        case IDGFXABOUT:
            get_selected_plugin(hwnd, IDC_COMBO_GFX)->about(hwnd);
            break;
        case IDM_INPUT_SETTINGS:
            get_selected_plugin(hwnd, IDC_COMBO_INPUT)->config(hwnd);
            break;
        case IDINPUTTEST:
            get_selected_plugin(hwnd, IDC_COMBO_INPUT)->test(hwnd);
            break;
        case IDINPUTABOUT:
            get_selected_plugin(hwnd, IDC_COMBO_INPUT)->about(hwnd);
            break;
        case IDM_AUDIO_SETTINGS:
            get_selected_plugin(hwnd, IDC_COMBO_SOUND)->config(hwnd);
            break;
        case IDSOUNDTEST:
            get_selected_plugin(hwnd, IDC_COMBO_SOUND)->test(hwnd);
            break;
        case IDSOUNDABOUT:
            get_selected_plugin(hwnd, IDC_COMBO_SOUND)->about(hwnd);
            break;
        case IDM_RSP_SETTINGS:
            get_selected_plugin(hwnd, IDC_COMBO_RSP)->config(hwnd);
            break;
        case IDRSPTEST:
            get_selected_plugin(hwnd, IDC_COMBO_RSP)->test(hwnd);
            break;
        case IDRSPABOUT:
            get_selected_plugin(hwnd, IDC_COMBO_RSP)->about(hwnd);
            break;
        case IDC_PLUGIN_DISCOVERY_INFO:
            DialogBox(g_main_ctx.hinst, MAKEINTRESOURCE(IDD_PLUGIN_DISCOVERY_RESULTS), hwnd, plugin_discovery_dlgproc);
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
                g_config.selected_video_plugin = plugin->path().string();
            }
            if (const auto plugin = get_selected_plugin(hwnd, IDC_COMBO_SOUND); plugin != nullptr)
            {
                g_config.selected_audio_plugin = plugin->path().string();
            }
            if (const auto plugin = get_selected_plugin(hwnd, IDC_COMBO_INPUT); plugin != nullptr)
            {
                g_config.selected_input_plugin = plugin->path().string();
            }
            if (const auto plugin = get_selected_plugin(hwnd, IDC_COMBO_RSP); plugin != nullptr)
            {
                g_config.selected_rsp_plugin = plugin->path().string();
            }
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

std::vector<t_options_group> get_static_option_groups()
{
    size_t id = 0;

    t_options_group folders_group = {.id = id++, .name = "Folders"};

    t_options_group interface_group = {.id = id++, .name = "Interface"};

    t_options_group statusbar_group = {.id = id++, .name = "Statusbar"};

    t_options_group piano_roll_group = {.id = id++, .name = "Piano Roll"};

    t_options_group seek_group = {.id = id++, .name = "Seek"};

    t_options_group capture_group = {.id = id++, .name = "Capture"};

    t_options_group core_group = {.id = id++, .name = "Core"};

    t_options_group vcr_group = {.id = id++, .name = "VCR"};

    t_options_group lua_group = {.id = id++, .name = "Lua"};

    t_options_group debug_group = {.id = id++, .name = "Debug"};

#define RPROP(T, x) t_options_item::t_readonly_property([] { return g_default_config.x; })

#define RWPROP(T, x, c)                                                                                                \
    t_options_item::t_readwrite_property([] { return g_config.x; },                                                    \
                                         [](const t_options_item::data_variant &value) {                               \
                                             g_config.x = std::get<T>(value);                                          \
                                             do                                                                        \
                                             {                                                                         \
                                                 c                                                                     \
                                             } while (0);                                                              \
                                         })

#define GENPROPS(T, x, ...) .current_value = RWPROP(T, x, __VA_ARGS__), .default_value = RPROP(T, x)

    folders_group.items.push_back({.type = t_options_item::Type::Folder,
                                   .group_id = folders_group.id,
                                   .name = "ROMs",
                                   .tooltip = "The path to the ROM folder.",
                                   GENPROPS(std::string, rom_directory)});
    folders_group.items.push_back({.type = t_options_item::Type::Folder,
                                   .group_id = folders_group.id,
                                   .name = "Plugins",
                                   .tooltip = "The path to the plugin folder.",
                                   GENPROPS(std::string, plugins_directory, { g_plugin_discovery_rescan = true; })});
    folders_group.items.push_back({.type = t_options_item::Type::Folder,
                                   .group_id = folders_group.id,
                                   .name = "Save Data",
                                   .tooltip = "The path to the save data folder.",
                                   GENPROPS(std::string, saves_directory),
                                   .is_readonly = [] { return g_main_ctx.core_ctx->vr_get_core_executing(); }});
    folders_group.items.push_back({.type = t_options_item::Type::Folder,
                                   .group_id = folders_group.id,
                                   .name = "Screenshots",
                                   .tooltip = "The path to the screenshot folder.",
                                   GENPROPS(std::string, screenshots_directory)});
    folders_group.items.push_back({.type = t_options_item::Type::Folder,
                                   .group_id = folders_group.id,
                                   .name = "Backup Folder",
                                   .tooltip = "The path to the movie backup folder.",
                                   GENPROPS(std::string, backups_directory)});

    interface_group.items.emplace_back(t_options_item{.type = t_options_item::Type::Enum,
                                                      .group_id = interface_group.id,
                                                      .name = "Theme",
                                                      .tooltip = "The UI theme to use.",
                                                      GENPROPS(int32_t, theme),
                                                      .possible_values = {
                                                          std::make_pair("Light", 0),
                                                          std::make_pair("Dark", 1),
                                                          std::make_pair("System", 2),
                                                      }});
    interface_group.items.emplace_back(t_options_item{.type = t_options_item::Type::Bool,
                                                      .group_id = interface_group.id,
                                                      .name = "Pause when unfocused",
                                                      .tooltip = "Pause emulation when the main window isn't in focus.",
                                                      GENPROPS(int32_t, is_unfocused_pause_enabled)});
    interface_group.items.emplace_back(
        t_options_item{.type = t_options_item::Type::Bool,
                       .group_id = interface_group.id,
                       .name = "Automatic Update Checking",
                       .tooltip = "Enables automatic update checking. Requires an internet connection.",
                       GENPROPS(int32_t, automatic_update_checking)});
    interface_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = interface_group.id,
        .name = "Silent Mode",
        .tooltip = "Suppresses all dialogs and chooses reasonable defaults for multiple-choice dialogs.\nCan cause "
                   "data loss during normal usage; only enable in automation scenarios!",
        GENPROPS(int32_t, silent_mode)});
    interface_group.items.emplace_back(
        t_options_item{.type = t_options_item::Type::Bool,
                       .group_id = interface_group.id,
                       .name = "Keep working directory",
                       .tooltip = "Keep the working directory specified by the caller program at startup.\nWhen "
                                  "disabled, mupen changes the working directory to its current path.",
                       GENPROPS(int32_t, keep_default_working_directory)});
    interface_group.items.emplace_back(
        t_options_item{.type = t_options_item::Type::Bool,
                       .group_id = interface_group.id,
                       .name = "Auto-increment Slot",
                       .tooltip = "Automatically increment the save slot upon saving a state.",
                       GENPROPS(int32_t, increment_slot)});

    statusbar_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Enum,
        .group_id = statusbar_group.id,
        .name = "Layout",
        .tooltip = "The statusbar layout preset.\nClassic - The legacy layout\nModern - The new layout containing "
                   "additional information\nModern+ - The new layout, but with a section for read-only status",
        GENPROPS(int32_t, statusbar_layout),
        .possible_values = {
            std::make_pair("Classic", (int32_t)t_config::StatusbarLayout::Classic),
            std::make_pair("Modern", (int32_t)t_config::StatusbarLayout::Modern),
            std::make_pair("Modern+", (int32_t)t_config::StatusbarLayout::ModernWithReadOnly),
        }});
    statusbar_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = statusbar_group.id,
        .name = "Zero-index",
        .tooltip = "Show indicies in the statusbar, such as VCR frame counts, relative to 0 instead of 1.",
        GENPROPS(int32_t, vcr_0_index)});
    statusbar_group.items.emplace_back(
        t_options_item{.type = t_options_item::Type::Bool,
                       .group_id = statusbar_group.id,
                       .name = "Scale down to fit window",
                       .tooltip = "Whether the statusbar is allowed to scale its segments down.",
                       GENPROPS(int32_t, statusbar_scale_down)});
    statusbar_group.items.emplace_back(
        t_options_item{.type = t_options_item::Type::Bool,
                       .group_id = statusbar_group.id,
                       .name = "Scale up to fill window",
                       .tooltip = "Whether the statusbar is allowed to scale its segments up.",
                       GENPROPS(int32_t, statusbar_scale_up)});
    piano_roll_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = piano_roll_group.id,
        .name = "Constrain edit to column",
        .tooltip = "Whether piano roll edits are constrained to the column they started on.",
        GENPROPS(int32_t, piano_roll_constrain_edit_to_column),
    });
    piano_roll_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Number,
        .group_id = piano_roll_group.id,
        .name = "History size",
        .tooltip = "Maximum size of the history list.",
        GENPROPS(int32_t, piano_roll_undo_stack_size),
    });
    piano_roll_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = piano_roll_group.id,
        .name = "Keep selection visible",
        .tooltip = "Whether the piano roll will try to keep the selection visible.",
        GENPROPS(int32_t, piano_roll_keep_selection_visible),
    });
    piano_roll_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = piano_roll_group.id,
        .name = "Keep playhead visible",
        .tooltip = "Whether the piano roll will try to keep the playhead visible.",
        GENPROPS(int32_t, piano_roll_keep_playhead_visible),
    });

    seek_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Number,
        .group_id = seek_group.id,
        .name = "Savestate Interval",
        .tooltip = "The interval at which to create savestates for seeking. Piano Roll is exclusively read-only if "
                   "this value is 0.\nHigher numbers will reduce the seek duration at cost of emulator performance, a "
                   "value of 1 is not allowed.\n0 - Seek savestate generation disabled\nRecommended: 100",
        GENPROPS(int32_t, core.seek_savestate_interval),
        .is_readonly = [] { return g_main_ctx.core_ctx->vcr_get_task() != task_idle; },
    });
    seek_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Number,
        .group_id = seek_group.id,
        .name = "Savestate Max Count",
        .tooltip = "The maximum amount of savestates to keep in memory for seeking.\nHigher numbers might cause an "
                   "out of memory exception.",
        GENPROPS(int32_t, core.seek_savestate_max_count),
    });

    capture_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Number,
        .group_id = capture_group.id,
        .name = "Delay",
        .tooltip = "Miliseconds to wait before capturing a frame. Useful for syncing with external programs.",
        GENPROPS(int32_t, capture_delay),
    });
    capture_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Enum,
        .group_id = capture_group.id,
        .name = "Encoder",
        .tooltip = "The encoder to use for capturing.",
        GENPROPS(int32_t, encoder_type),
        .possible_values =
            {
                std::make_pair("VFW", (int32_t)t_config::EncoderType::VFW),
                std::make_pair("FFmpeg", (int32_t)t_config::EncoderType::FFmpeg),
            },
        .is_readonly = [] { return CaptureManager::is_capturing(); },
    });
    capture_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Enum,
        .group_id = capture_group.id,
        .name = "Mode",
        .tooltip = "The video source to use for capturing video frames.\nPlugin - Captures frames solely from the "
                   "video plugin\nWindow - Captures frames from the main window\nScreen - Captures screenshots of the "
                   "current display and crops them to Mupen\nHybrid - Combines video plugin capture and internal Lua "
                   "composition (recommended)",
        GENPROPS(int32_t, capture_mode),
        .possible_values =
            {
                std::make_pair("Plugin", 0),
                std::make_pair("Window", 1),
                std::make_pair("Screen", 2),
                std::make_pair("Hybrid", 3),
            },
        .is_readonly = [] { return CaptureManager::is_capturing(); },
    });
    capture_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = capture_group.id,
        .name = "Stop at Movie End",
        .tooltip = "Whether capturing is automatically stopped when a movie ends.",
        GENPROPS(int32_t, stop_capture_at_movie_end),
    });
    capture_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Enum,
        .group_id = capture_group.id,
        .name = "Sync",
        .tooltip = "The strategy to use for synchronizing video and audio during capture.\nNone - No "
                   "synchronization\nAudio - Audio is synchronized to video\nVideo - Video is synchronized to audio",
        GENPROPS(int32_t, synchronization_mode),
        .possible_values =
            {
                std::make_pair("None", 0),
                std::make_pair("Audio", 1),
                std::make_pair("Video", 2),
            },
        .is_readonly = [] { return CaptureManager::is_capturing(); },
    });
    capture_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::String,
        .group_id = capture_group.id,
        .name = "FFmpeg Path",
        .tooltip = "The path to the FFmpeg executable to use for capturing.",
        GENPROPS(std::string, ffmpeg_path),
        .is_readonly = [] { return CaptureManager::is_capturing(); },
    });
    capture_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::String,
        .group_id = capture_group.id,
        .name = "FFmpeg Arguments",
        .tooltip = "FFmpeg arguments to be passed to FFmpeg when capturing.",
        GENPROPS(std::string, ffmpeg_options),
        .is_readonly = [] { return CaptureManager::is_capturing(); },
    });

    core_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Enum,
        .group_id = core_group.id,
        .name = "Type",
        .tooltip =
            "The core type to utilize for emulation.\nInterpreter - Slow and relatively accurate\nDynamic Recompiler "
            "- Fast, possibly less accurate, and only for x86 processors\nPure Interpreter - Very slow and accurate",
        GENPROPS(int32_t, core.core_type),
        .possible_values =
            {
                std::make_pair("Interpreter", 0),
                std::make_pair("Dynamic Recompiler", 1),
                std::make_pair("Pure Interpreter", 2),
            },
        .is_readonly = [] { return g_main_ctx.core_ctx->vr_get_launched(); },
    });
    core_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = core_group.id,
        .name = "Undo Savestate Load",
        .tooltip = "Whether undo savestate load functionality is enabled.",
        GENPROPS(int32_t, core.st_undo_load),
    });
    core_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Number,
        .group_id = core_group.id,
        .name = "Max Lag Frames",
        .tooltip = "The maximum amount of lag frames before the core emits a warning\n0 - Disabled",
        GENPROPS(int32_t, core.max_lag),
    });
    core_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = core_group.id,
        .name = "WiiVC Mode",
        .tooltip = "Enables WiiVC emulation.",
        GENPROPS(int32_t, core.wii_vc_emulation),
    });
    core_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = core_group.id,
        .name = "Emulate RCP Lag",
        .tooltip = "Enables RCP lag emulation, which is a more accurate emulation of lag frames.",
        GENPROPS(int32_t, core.rcp_lag_emulation),
    });
    core_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Number,
        .group_id = core_group.id,
        .name = "CPU Counter Factor",
        .tooltip = "The CPU counter factor. Higher values reduce effective lag.",
        GENPROPS(double, core.cpu_cf),
    });
    core_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Number,
        .group_id = core_group.id,
        .name = "RCP Lag Factor",
        .tooltip = "The RCP lag factor. Lower values reduce effective lag, higher values increase it.",
        GENPROPS(double, core.rcp_lag_factor),
    });
    core_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = core_group.id,
        .name = "Emulate Float Crashes",
        .tooltip = "Emulate float operation-related crashes which would also crash on real hardware",
        GENPROPS(int32_t, core.float_exception_emulation),
    });
    core_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = core_group.id,
        .name = "Emulate SD Card",
        .tooltip =
            "Enable SD card emulation.\nRequires a VHD-formatted SD card file named card.vhd in the save data folder.",
        GENPROPS(int32_t, core.use_summercart),
    });
    core_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = core_group.id,
        .name = "Instant Savestate Update",
        .tooltip = "Saves and loads game graphics to savestates to allow instant graphics updates when loading "
                   "savestates.\nGreatly increases savestate saving and loading time.",
        GENPROPS(int32_t, core.st_screenshot),
    });
    core_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = core_group.id,
        .name = "Fast Savestates",
        .tooltip = "Compress savestates using LZ4, faster than GZip.\nDisable to create savestates that are "
                   "compatible with older Mupen versions.",
        GENPROPS(int32_t, core.st_lz4),
    });
    core_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Number,
        .group_id = core_group.id,
        .name = "ROM Cache Size",
        .tooltip = "Size of the ROM cache.\nImproves ROM loading performance at the cost of data staleness and high "
                   "memory usage.\n0 - Disabled\nn - Maximum of n ROMs kept in cache",
        GENPROPS(int32_t, core.rom_cache_size),
    });

    vcr_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = vcr_group.id,
        .name = "Movie Backups",
        .tooltip = "Generate a backup of the currently recorded movie when loading a savestate.\nBackups are saved in "
                   "the backups folder.",
        GENPROPS(int32_t, core.vcr_backups),
    });
    vcr_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = vcr_group.id,
        .name = "Extended Movie Format",
        .tooltip = "Whether movies are written using the new extended format.\nUseful when opening movies in external "
                   "programs which don't handle the new format correctly.\nIf disabled, the extended format sections "
                   "are set to 0.",
        GENPROPS(int32_t, core.vcr_write_extended_format),
    });
    vcr_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = vcr_group.id,
        .name = "Record Resets",
        .tooltip = "Record manually performed resets to the current movie.\nThese resets will be repeated when the "
                   "movie is played back.",
        GENPROPS(int32_t, core.is_reset_recording_enabled),
    });

    lua_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Enum,
        .group_id = lua_group.id,
        .name = "Presenter",
        .tooltip =
            "The presenter type to use for displaying and capturing Lua graphics.\nRecommended: DirectComposition",
        GENPROPS(int32_t, presenter_type),
        .possible_values =
            {
                std::make_pair("DirectComposition", (int32_t)t_config::PresenterType::DirectComposition),
                std::make_pair("GDI", (int32_t)t_config::PresenterType::GDI),
            },
        .is_readonly = [] { return !g_lua_environments.empty(); },
    });
    lua_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = lua_group.id,
        .name = "Lazy Renderer Initialization",
        .tooltip =
            "Enables lazy Lua renderer initialization. Greatly speeds up start and stop times for certain scripts.",
        GENPROPS(int32_t, lazy_renderer_init),
        .is_readonly = [] { return !g_lua_environments.empty(); },
    });

    debug_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = debug_group.id,
        .name = "Audio Delay",
        .tooltip = "Whether to delay audio interrupts.",
        GENPROPS(int32_t, core.is_audio_delay_enabled),
    });
    debug_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = debug_group.id,
        .name = "Compiled Jump",
        .tooltip = "Whether the Dynamic Recompiler core compiles jumps.",
        GENPROPS(int32_t, core.is_compiled_jump_enabled),
    });
    debug_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = debug_group.id,
        .name = "Accurate C.EQ.S",
        .tooltip = "Whether the C_EQ_S instruction produces `(NaN == any) == false` when using the Dynamic Recompiler "
                   "core.\nThe legacy behaviour is `(NaN == any) == true`, but this option is kept for "
                   "backwards-compatibility.",
        GENPROPS(int32_t, core.c_eq_s_nan_accurate),
        .is_readonly = [] { return g_main_ctx.core_ctx->vr_get_launched(); },
    });
    debug_group.items.emplace_back(t_options_item{
        .type = t_options_item::Type::Bool,
        .group_id = debug_group.id,
        .name = "Accurate RDP Completion",
        .tooltip = "Whether RDP task completion is signalled after RSP task completion instead of at the same "
                   "instant.\nThe RDP consumes the RSP's output, so on hardware it always finishes later. The legacy "
                   "behaviour signals both at once, but is kept as the default for backwards-compatibility.\nEnabling "
                   "this desynchronizes movies recorded with the legacy timing.",
        GENPROPS(int32_t, core.accurate_rdp_completion),
        .is_readonly = [] { return g_main_ctx.core_ctx->vr_get_launched(); },
    });

    return {folders_group, interface_group, statusbar_group, piano_roll_group, seek_group,
            capture_group, core_group,      vcr_group,       lua_group,        debug_group};
}

LRESULT CALLBACK inline_edit_subclass_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id,
                                           DWORD_PTR ref_data)
{
    switch (msg)
    {
    case WM_GETDLGCODE: {
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
        break;
    default:
        break;
    }

def:
    return DefSubclassProc(hwnd, msg, wparam, lparam);

apply:

    const auto text = get_window_text(hwnd).value_or("");
    SendMessage(GetParent(hwnd), WM_EDIT_END, 0, (LPARAM)text.c_str());

    DestroyWindow(hwnd);

    goto def;
}

/**
 * Advances a listview's selection by one.
 */
void advance_listview_selection(HWND lvhwnd)
{
    int32_t i = ListView_GetNextItem(lvhwnd, -1, LVNI_SELECTED);
    if (i == -1) return;
    ListView_SetItemState(lvhwnd, i, 0, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_SetItemState(lvhwnd, i + 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(lvhwnd, i + 1, false);
}

INT_PTR CALLBACK generic_tab_proc(const HWND hwnd, const UINT message, const WPARAM w_param, const LPARAM l_param)
{
    const auto lpnmhdr = reinterpret_cast<LPNMHDR>(l_param);
    auto ctx = (t_tab_context *)GetProp(hwnd, "tab_context");

    const auto base_result = base_pageproc(hwnd, message, w_param, l_param);
    if (base_result) return base_result;

    switch (message)
    {
    case WM_INITDIALOG: {
        const auto ps = (PROPSHEETPAGE *)l_param;
        SetProp(hwnd, "tab_context", (HANDLE)ps->lParam);

        ctx = (t_tab_context *)GetProp(hwnd, "tab_context");
        ctx->hwnd = hwnd;
        WinDarkMode::attach(hwnd);
        return TRUE;
    }
    case WM_EDIT_END: {
        auto option_item = g_option_items[ctx->item_index_map.at(ctx->edit_option_item_index)];
        auto str = reinterpret_cast<char *>(l_param);

        if (option_item.type == t_options_item::Type::Number)
        {
            try
            {
                option_item.current_value.set(parse_number_value(str, option_item.current_value.get()));
            }
            catch (...)
            {
                // ignored
            }
        }
        else
        {
            option_item.current_value.set(std::string(str));
        }

        ListView_Update(ctx->lv_hwnd, ctx->edit_option_item_index);

        break;
    }
    case WM_CONTEXTMENU: {
        int32_t i = ListView_GetNextItem(ctx->lv_hwnd, -1, LVNI_SELECTED);
        if (i == -1) break;

        LVITEM item = {0};
        item.mask = LVIF_PARAM;
        item.iItem = i;
        ListView_GetItem(ctx->lv_hwnd, &item);

        auto &option_item = g_option_items[ctx->item_index_map.at(item.lParam)];

        auto readonly = option_item.is_readonly();

        HMENU h_menu = CreatePopupMenu();
        AppendMenu(h_menu, MF_STRING | (readonly ? MF_DISABLED : MF_ENABLED), 1, "Reset to default");
        AppendMenu(h_menu, MF_STRING, 2, "More info...");
        AppendMenu(h_menu, MF_SEPARATOR, 100, "");
        switch (option_item.type)
        {
        case t_options_item::Type::Hotkey:
            AppendMenu(h_menu, MF_STRING, 4, "Clear");
            AppendMenu(h_menu, MF_SEPARATOR, 100, "");
            break;
        case t_options_item::Type::Folder:
            AppendMenu(h_menu, MF_STRING, 5, "Show in Explorer");
            AppendMenu(h_menu, MF_SEPARATOR, 100, "");
            break;
        default:
            break;
        }
        AppendMenu(h_menu, MF_STRING, 3, "Reset all to default");

        const int offset = TrackPopupMenuEx(h_menu, TPM_RETURNCMD | TPM_NONOTIFY, GET_X_LPARAM(l_param),
                                            GET_Y_LPARAM(l_param), hwnd, 0);

        if (offset < 0)
        {
            break;
        }

        switch (offset)
        {
        case 1:
            option_item.reset_to_default();
            ListView_Update(ctx->lv_hwnd, i);
            break;
        case 2:
            g_dialog_service->show_dialog(option_item.get_friendly_info(), option_item.name, fsvc_information, hwnd);
            break;
        case 4:
            option_item.current_value.set(Hotkey::make_empty());
            ListView_Update(ctx->lv_hwnd, i);
            break;
        case 5: {
            const auto path = std::get<std::string>(option_item.current_value.get());
            ShellExecute(hwnd, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            break;
        }
        case 3: {
            // If some settings can't be changed, we'll bail
            bool can_all_be_changed = true;

            for (const auto &item : g_option_items)
            {
                if (!item.is_readonly()) continue;

                can_all_be_changed = false;
                break;
            }

            if (!can_all_be_changed)
            {
                g_dialog_service->show_dialog(
                    "Some settings can't be reset, as they are currently read-only. Try again "
                    "with emulation stopped.\nNo changes have been made to the settings.",
                    "Reset all to default", fsvc_warning, hwnd);
                break;
            }

            const auto result = g_dialog_service->show_ask_dialog(
                VIEW_DLG_RESET_SETTINGS,
                "Reset all settings to their default values?\nThis will reset settings on all pages.",
                "Reset all to default", false, hwnd);

            if (!result)
            {
                break;
            }

            for (auto &v : g_option_items)
            {
                v.reset_to_default();
            }

            ListView_RedrawItems(ctx->lv_hwnd, 0, ListView_GetItemCount(ctx->lv_hwnd));
            break;
        }
        default:
            break;
        }

        DestroyMenu(h_menu);
    }
    break;
    case WM_NCDESTROY:
        RemoveProp(hwnd, "tab_context");
        delete ctx;
        ctx = nullptr;
        break;
    case WM_NOTIFY: {
        if (lpnmhdr->code == PSN_SETACTIVE)
        {
            g_config.settings_tab = ctx->tab_index;

            RECT grid_rect{};
            GetClientRect(hwnd, &grid_rect);

            std::vector<SettingsListView::t_group> groups;
            for (const auto &wanted_group : ctx->groups)
            {
                auto it = std::find_if(g_option_groups.begin(), g_option_groups.end(),
                                       [&](const t_options_group &group) { return group.name == wanted_group; });
                if (it != g_option_groups.end())
                {
                    groups.emplace_back(it->id, it->name);
                }
            }

            std::vector<SettingsListView::t_item> items;
            for (size_t i = 0; i < g_option_items.size(); ++i)
            {
                const auto &item = g_option_items[i];

                if (std::find(ctx->groups.begin(), ctx->groups.end(), g_option_groups[item.group_id].name) ==
                    ctx->groups.end())
                {
                    continue;
                }

                ctx->item_index_map[items.size()] = i;
                items.emplace_back(item.group_id, item.get_name());
            }

            auto get_item_tooltip = [=](size_t i) -> std::string {
                const auto &global_item = g_option_items[ctx->item_index_map.at(i)];
                return global_item.tooltip;
            };

            auto edit_start = [=](size_t i) {
                auto &global_item = g_option_items[ctx->item_index_map.at(i)];

                // TODO: Perhaps gray out readonly values too?
                if (global_item.is_readonly())
                {
                    return false;
                }

                // We use the default detached editing, except for numbers, which are edited inline.
                if (global_item.type != t_options_item::Type::Number)
                {
                    (void)global_item.edit(ctx->hwnd);
                    ListView_RedrawItems(ctx->lv_hwnd, 0, ListView_GetItemCount(ctx->lv_hwnd));
                    return true;
                }

                if (ctx->edit_hwnd)
                {
                    DestroyWindow(ctx->edit_hwnd);
                }

                ctx->edit_option_item_index = i;

                RECT item_rect{};
                ListView_GetSubItemRect(ctx->lv_hwnd, i, 1, LVIR_LABEL, &item_rect);

                RECT lv_rect{};
                GetClientRect(ctx->lv_hwnd, &lv_rect);

                item_rect.right = lv_rect.right;

                ctx->edit_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                                item_rect.left, item_rect.top, item_rect.right - item_rect.left,
                                                item_rect.bottom - item_rect.top, ctx->hwnd, 0, g_main_ctx.hinst, 0);
                SendMessage(ctx->edit_hwnd, WM_SETFONT, (WPARAM)SendMessage(ctx->lv_hwnd, WM_GETFONT, 0, 0), 0);

                SetWindowSubclass(ctx->edit_hwnd, inline_edit_subclass_proc, 0, 0);
                WinDarkMode::attach(ctx->edit_hwnd);

                const auto value = global_item.current_value.get();
                Edit_SetText(ctx->edit_hwnd, to_str_default(get_number_value(value)).c_str());

                PostMessage(ctx->hwnd, WM_NEXTDLGCTL, (WPARAM)ctx->edit_hwnd, TRUE);

                ListView_RedrawItems(ctx->lv_hwnd, 0, ListView_GetItemCount(ctx->lv_hwnd));
                return true;
            };

            auto get_item_image = [=](size_t i) {
                const auto &global_item = g_option_items[ctx->item_index_map.at(i)];

                int32_t image = global_item.initial_value.get() == global_item.current_value.get() ? 50 : 1;

                if (global_item.is_readonly())
                {
                    image = 0;
                }

                return image;
            };

            auto get_item_text = [=](size_t i, size_t subitem) {
                const auto &global_item = g_option_items[ctx->item_index_map.at(i)];

                if (subitem == 0)
                {
                    return global_item.get_name();
                }

                return global_item.get_value_name();
            };

            ctx->lv_hwnd = SettingsListView::create({
                .dlg_hwnd = hwnd,
                .rect = grid_rect,
                .on_edit_start = edit_start,
                .groups = groups,
                .items = items,
                .get_item_tooltip = get_item_tooltip,
                .get_item_text = get_item_text,
                .get_item_image = get_item_image,
            });
        }

        if (lpnmhdr->code == PSN_KILLACTIVE)
        {
            if (ctx->lv_hwnd) DestroyWindow(ctx->lv_hwnd);
            if (ctx->edit_hwnd) DestroyWindow(ctx->edit_hwnd);
        }

        return SettingsListView::notify(hwnd, ctx->lv_hwnd, l_param, w_param);
    }
    default:
        return FALSE;
    }
    return TRUE;
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
    std::vector<std::string> unique_group_names;
    const auto all_actions = ActionManager::get_actions_matching_filter("*");

    for (const auto &path : all_actions)
    {
        std::vector<std::string> segments = ActionManager::get_segments(path);

        if (segments.size() <= 1)
        {
            continue;
        }

        segments.pop_back();

        std::string group_name;
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

    for (const auto &name : unique_group_names)
    {
        groups.emplace_back(t_options_group{.id = base_id++, .name = name});
    }

    return groups;
}

static int CALLBACK prop_sheet_callback(HWND hwnd, UINT msg, LPARAM lparam)
{
    switch (msg)
    {
    case PSCB_INITIALIZED: {
        WinDarkMode::attach(hwnd);
        break;
    }
    }
    return 0;
}

void ConfigDialog::show_app_settings()
{
    const auto groups = get_option_groups();
    g_option_groups = groups;
    g_option_items.clear();
    for (const auto &group : groups)
    {
        for (const auto &item : group.items)
        {
            g_option_items.emplace_back(item);
        }
    }

    std::vector<PROPSHEETPAGE> psp;

    psp.push_back({
        .pszTemplate = MAKEINTRESOURCE(IDD_SETTINGS_PLUGINS),
        .pszTitle = "Plugins",
        .pfnDlgProc = plugins_cfg,
    });

    psp.push_back({
        .pszTemplate = MAKEINTRESOURCE(IDD_SETTINGS_GENERAL),
        .pszTitle = "Folders",
        .pfnDlgProc = generic_tab_proc,
        .lParam = (LPARAM) new t_tab_context({.tab_index = psp.size(), .groups = {"Folders"}}),
    });

    psp.push_back({
        .pszTemplate = MAKEINTRESOURCE(IDD_SETTINGS_GENERAL),
        .pszTitle = "Visual",
        .pfnDlgProc = generic_tab_proc,
        .lParam =
            (LPARAM) new t_tab_context({.tab_index = psp.size(), .groups = {"Interface", "Statusbar", "Piano Roll"}}),
    });

    psp.push_back({
        .pszTemplate = MAKEINTRESOURCE(IDD_SETTINGS_GENERAL),
        .pszTitle = "Emulation",
        .pfnDlgProc = generic_tab_proc,
        .lParam = (LPARAM) new t_tab_context({.tab_index = psp.size(), .groups = {"Core", "VCR", "Seek", "Debug"}}),
    });

    psp.push_back({
        .pszTemplate = MAKEINTRESOURCE(IDD_SETTINGS_GENERAL),
        .pszTitle = "Capture",
        .pfnDlgProc = generic_tab_proc,
        .lParam = (LPARAM) new t_tab_context({.tab_index = psp.size(), .groups = {"Capture"}}),
    });

    psp.push_back({
        .pszTemplate = MAKEINTRESOURCE(IDD_SETTINGS_GENERAL),
        .pszTitle = "Lua",
        .pfnDlgProc = generic_tab_proc,
        .lParam = (LPARAM) new t_tab_context({.tab_index = psp.size(), .groups = {"Lua"}}),
    });

    std::vector<std::string> hotkey_groups;
    for (size_t i = g_static_option_groups.size(); i < g_option_groups.size(); ++i)
        hotkey_groups.emplace_back(g_option_groups[i].name);
    psp.push_back({
        .pszTemplate = MAKEINTRESOURCE(IDD_SETTINGS_GENERAL),
        .pszTitle = "Hotkeys",
        .pfnDlgProc = generic_tab_proc,
        .lParam = (LPARAM) new t_tab_context({.tab_index = psp.size(), .groups = hotkey_groups}),
    });

    for (auto &page : psp)
    {
        page.dwSize = sizeof(PROPSHEETPAGE);
        page.dwFlags = PSP_USETITLE;
        page.hInstance = g_main_ctx.hinst;
    }

    PROPSHEETHEADER psh = {0};
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP | PSH_USECALLBACK;
    psh.pfnCallback = prop_sheet_callback;
    psh.hwndParent = g_main_ctx.hwnd;
    psh.hInstance = g_main_ctx.hinst;
    psh.pszCaption = "Settings";
    psh.nPages = psp.size();
    psh.nStartPage = g_config.settings_tab;
    psh.ppsp = (LPCPROPSHEETPAGE)psp.data();

    g_prev_config = g_config;

    const bool cancelled = !PropertySheet(&psh);

    if (cancelled)
    {
        g_config = g_prev_config;
    }

    Config::apply_and_save();
    Messenger::broadcast<Messenger::Message::ConfigLoaded>();
}

std::vector<t_options_group> ConfigDialog::get_option_groups()
{
    if (g_static_option_groups.empty())
    {
        g_static_option_groups = get_static_option_groups();
    }

    auto dynamic_option_groups = generate_hotkey_groups(g_static_option_groups.back().id + 1);
    for (auto &group : dynamic_option_groups)
    {
        const auto uname = group.name;
        const auto actions = ActionManager::get_actions_matching_filter(std::format("{} > *", uname));
        group.items.reserve(group.items.size() + actions.size());

        for (const auto &action : actions)
        {
            const auto action_segments = ActionManager::get_segments(action);
            const auto group_segments = ActionManager::get_segments(uname);

            if (action_segments.at(action_segments.size() - 2) != group_segments.back())
            {
                continue;
            }

            const t_options_item item = {
                .type = t_options_item::Type::Hotkey,
                .group_id = group.id,
                .name = action,
                .current_value = t_options_item::t_readwrite_property([=] { return g_config.hotkeys.at(action); },
                                                                      [=](const t_options_item::data_variant &value) {
                                                                          g_config.hotkeys[action] =
                                                                              std::get<Hotkey>(value);
                                                                      }),
                .default_value =
                    t_options_item::t_readonly_property([=] { return g_config.inital_hotkeys.at(action); }),
            };

            group.items.emplace_back(item);
        }
    }

    // We beautify the names here, a bit annoying because we have to reconstruct them
    for (auto &option_group : dynamic_option_groups)
    {
        auto segments = ActionManager::get_segments(option_group.name);
        for (auto &segment : segments)
        {
            segment = ActionManager::get_display_name(segment, true);
        }
        const auto name = StrUtils::join_string(segments, std::format(" {} ", ActionManager::SEGMENT_SEPARATOR));
        option_group.name = name;
    }

    std::vector<t_options_group> option_groups;
    option_groups.reserve(g_static_option_groups.size() + dynamic_option_groups.size());
    option_groups.insert(option_groups.end(), g_static_option_groups.begin(), g_static_option_groups.end());
    option_groups.insert(option_groups.end(), dynamic_option_groups.begin(), dynamic_option_groups.end());

    // Arm all initial values
    for (auto &option_group : option_groups)
    {
        for (auto &option_item : option_group.items)
        {
            const auto initial_value = option_item.current_value.get();
            option_item.initial_value = t_options_item::t_readonly_property([=] { return initial_value; });
        }
    }

    return option_groups;
}
