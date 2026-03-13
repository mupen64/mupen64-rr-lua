/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors
 * (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Config.h>
#include <Uxtheme.h>
#include <components/RomBrowser.h>
#include <components/Statusbar.h>
#include <components/AppActions.h>
#include <Messenger.h>

namespace RomBrowser
{

struct t_rombrowser_context
{
    HWND hwnd{};
    std::vector<t_simple_rom_info> discovered_roms;
    std::mutex mutex;
};

static t_rombrowser_context g_ctx{};

std::vector<std::filesystem::path> discover_roms()
{
    const auto abs_rom_directory =
        std::filesystem::weakly_canonical(IOUtils::exe_path_cached().parent_path() / g_config.rom_directory);

    if (!std::filesystem::exists(abs_rom_directory) || !std::filesystem::is_directory(abs_rom_directory))
    {
        return {};
    }

    std::vector<std::filesystem::path> rom_paths;
    std::vector<std::filesystem::path> filtered_rom_paths;

    // we aggregate all file paths and only filter them after we're done
    if (std::filesystem::is_directory(abs_rom_directory))
    {
        // this filters a directory iterator to get the paths of all regular files.
        auto only_file_paths =
            std::views::filter([](const std::filesystem::directory_entry &entry) { return entry.is_regular_file(); }) |
            std::views::transform([](const std::filesystem::directory_entry &entry) { return entry.path(); });

        if (g_config.is_rombrowser_recursion_enabled)
        {
            auto path_iter = std::filesystem::recursive_directory_iterator(abs_rom_directory) | only_file_paths;
            std::ranges::copy(path_iter, std::back_inserter(rom_paths));
        }
        else
        {
            auto path_iter = std::filesystem::directory_iterator(abs_rom_directory) | only_file_paths;
            std::ranges::copy(path_iter, std::back_inserter(rom_paths));
        }
    }
    else
    {
        g_main_ctx.core.log_warn("ROM directory does not exist; no ROMs will show in the ROM browser");
    }

    // logically this should be bundled into the filter pipeline but I'm too lazy
    std::ranges::copy_if(rom_paths, std::back_inserter(filtered_rom_paths), [](const auto val) {
        wchar_t c_extension[_MAX_EXT] = {0};
        if (_wsplitpath_s(val.c_str(), nullptr, 0, nullptr, 0, nullptr, 0, c_extension, _countof(c_extension)))
        {
            return false;
        }
        return MiscHelpers::iequals(c_extension, L".z64") || MiscHelpers::iequals(c_extension, L".n64") ||
               MiscHelpers::iequals(c_extension, L".v64") || MiscHelpers::iequals(c_extension, L".rom");
    });
    return filtered_rom_paths;
}

int CALLBACK rombrowser_compare(LPARAM lParam1, LPARAM lParam2, LPARAM _)
{
    auto first = g_ctx.discovered_roms[g_config.rombrowser_sort_ascending ? lParam1 : lParam2];
    auto second = g_ctx.discovered_roms[g_config.rombrowser_sort_ascending ? lParam2 : lParam1];

    int32_t result = 0;

    switch (g_config.rombrowser_sorted_column)
    {
    case 0:
        result = first.header.Country_code - second.header.Country_code;
        break;
    case 1:
        // BUG: these are not null terminated!!!
        result = _strcmpi((const char *)first.header.nom, (const char *)second.header.nom);
        break;
    case 2:
        result = _strcmpi((const char *)first.path.c_str(), (const char *)second.path.c_str());
        break;
    case 3:
        result = first.size - second.size;
        break;
    default:
        assert(false);
        break;
    }

    if (result > 1)
    {
        result = 1;
    }
    if (result < -1)
    {
        result = -1;
    }

    return result;
}

void rombrowser_update_sort()
{
    ListView_SortItems(g_ctx.hwnd, rombrowser_compare, nullptr);
}

void rombrowser_create()
{
    assert(g_ctx.hwnd == nullptr);

    RECT rcl{}, rtool{}, rstatus{};
    GetClientRect(g_main_ctx.hwnd, &rcl);
    GetWindowRect(Statusbar::hwnd(), &rstatus);

    g_ctx.hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL,
                                WS_TABSTOP | WS_VISIBLE | WS_CHILD | LVS_SINGLESEL | LVS_REPORT | LVS_SHOWSELALWAYS, 0,
                                rtool.bottom - rtool.top, rcl.right - rcl.left,
                                rcl.bottom - rcl.top - rtool.bottom + rtool.top - rstatus.bottom + rstatus.top,
                                g_main_ctx.hwnd, (HMENU)IDC_ROMLIST, g_main_ctx.hinst, NULL);
    ListView_SetExtendedListViewStyle(g_ctx.hwnd, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    // Explorer theme is better than default, because it has selection highlight
    SetWindowTheme(g_ctx.hwnd, L"Explorer", NULL);

    const HIMAGELIST h_small = ImageList_Create(16, 16, ILC_COLORDDB | ILC_MASK, 11, 0);
    HICON h_icon;

#define ADD_ICON(id)                                                                                                   \
    h_icon = LoadIcon(g_main_ctx.hinst, MAKEINTRESOURCE(id));                                                          \
    ImageList_AddIcon(h_small, h_icon)

    ADD_ICON(IDI_GERMANY);
    ADD_ICON(IDI_USA);
    ADD_ICON(IDI_JAPAN);
    ADD_ICON(IDI_EUROPE);
    ADD_ICON(IDI_AUSTRALIA);
    ADD_ICON(IDI_ITALIA);
    ADD_ICON(IDI_FRANCE);
    ADD_ICON(IDI_SPAIN);
    ADD_ICON(IDI_UNKNOWN);
    ADD_ICON(IDI_DEMO);
    ADD_ICON(IDI_BETA);

    ListView_SetImageList(g_ctx.hwnd, h_small, LVSIL_SMALL);

    LVCOLUMN lv_column = {0};
    lv_column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    lv_column.iImage = 1;
    lv_column.cx = g_config.rombrowser_column_widths[0];
    ListView_InsertColumn(g_ctx.hwnd, 0, &lv_column);

    lv_column.pszText = const_cast<LPWSTR>(L"Name");
    lv_column.cx = g_config.rombrowser_column_widths[1];
    ListView_InsertColumn(g_ctx.hwnd, 1, &lv_column);

    lv_column.pszText = const_cast<LPWSTR>(L"Filename");
    lv_column.cx = g_config.rombrowser_column_widths[2];
    ListView_InsertColumn(g_ctx.hwnd, 2, &lv_column);

    lv_column.pszText = const_cast<LPWSTR>(L"Size");
    lv_column.cx = g_config.rombrowser_column_widths[3];
    ListView_InsertColumn(g_ctx.hwnd, 3, &lv_column);

    BringWindowToTop(g_ctx.hwnd);
}

int32_t rombrowser_country_code_to_image_index(uint16_t country_code)
{
    switch (country_code & 0xFF)
    {
    case 0:
        return 9;
    case '7':
        return 10;
    case 0x44:
        return 0; // IDI_GERMANY
    case 0x45:
        return 1; // IDI_USA
    case 0x4A:
        return 2; // IDI_JAPAN
    case 0x20:
    case 0x21:
    case 0x38:
    case 0x70:
    case 0x50:
    case 0x58:
        return 3; // IDI_EUROPE
    case 0x55:
        return 4; // IDI_AUSTRALIA
    case 'I':
        return 5; // Italy
    case 0x46:    // IDI_FRANCE
        return 6;
    case 'S': // SPAIN
        return 7;
    default:
        return 8; // IDI_N64CART
    }
}

static void build_impl()
{
    std::unique_lock lock(g_ctx.mutex, std::try_to_lock);
    if (!lock.owns_lock())
    {
        g_view_logger->info("[Rombrowser] build_impl busy!");
        return;
    }

    const auto start_time = std::chrono::high_resolution_clock::now();

    g_main_ctx.dispatcher->invoke([] {
        SendMessage(g_ctx.hwnd, WM_SETREDRAW, FALSE, 0);
        ListView_DeleteAllItems(g_ctx.hwnd);
    });

    g_ctx.discovered_roms.clear();
    const auto rom_paths = discover_roms();

    std::vector<t_simple_rom_info> results(rom_paths.size());

    auto worker = [&](size_t begin, size_t end) {
        for (size_t j = begin; j < end; ++j)
        {
            const auto &path = rom_paths[j];

            FILE *f = nullptr;
            if (_wfopen_s(&f, path.c_str(), L"rb")) continue;

            fseek(f, 0, SEEK_END);
            size_t len = ftell(f);
            fseek(f, 0, SEEK_SET);

            t_simple_rom_info entry{};
            entry.path = path;
            entry.size = len;

            if (len > sizeof(core_rom_header))
            {
                core_rom_header header{};
                fread(&header, sizeof(header), 1, f);

                g_main_ctx.core_ctx->vr_byteswap((uint8_t *)&header);

                MiscHelpers::strtrim((char *)header.nom, sizeof(header.nom));
                header.nom[sizeof(header.nom) - 1] = '\0';

                entry.header = header;
            }

            fclose(f);

            results[j] = std::move(entry);
        }
    };

    // Split the work across two threads since we're not heavily IO-bound on small reads.
    size_t mid = rom_paths.size() / 2;
    std::thread t1(worker, 0, mid);
    std::thread t2(worker, mid, rom_paths.size());
    t1.join();
    t2.join();

    g_ctx.discovered_roms = std::move(results);

    g_main_ctx.dispatcher->invoke([] {
        for (size_t i = 0; i < g_ctx.discovered_roms.size(); i++)
        {
            const auto &entry = g_ctx.discovered_roms[i];

            LV_ITEM lv_item = {0};
            lv_item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
            lv_item.pszText = LPSTR_TEXTCALLBACK;
            lv_item.lParam = i;
            lv_item.iItem = i;
            lv_item.iImage = rombrowser_country_code_to_image_index(entry.header.Country_code);
            ListView_InsertItem(g_ctx.hwnd, &lv_item);
        }
        rombrowser_update_sort();

        SendMessage(g_ctx.hwnd, WM_SETREDRAW, TRUE, 0);
    });

    g_view_logger->info("Rombrowser loading took {}ms",
                        static_cast<int>((std::chrono::high_resolution_clock::now() - start_time).count() / 1'000'000));
}

void build()
{
    std::thread(build_impl).detach();
}

void rombrowser_update_size()
{
    if (g_main_ctx.core_ctx->vr_get_launched()) return;
    if (!IsWindow(g_ctx.hwnd)) return;

    // TODO: clean up this mess
    RECT rc, rc_main;
    WORD n_width, n_height;
    int32_t y = 0;
    GetClientRect(g_main_ctx.hwnd, &rc_main);
    n_width = rc_main.right - rc_main.left;
    n_height = rc_main.bottom - rc_main.top;
    if (Statusbar::hwnd())
    {
        GetWindowRect(Statusbar::hwnd(), &rc);
        n_height -= (WORD)(rc.bottom - rc.top);
    }
    MoveWindow(g_ctx.hwnd, 0, y, n_width, n_height - y, TRUE);
}

void invoke_selected_item()
{
    int32_t i = ListView_GetNextItem(g_ctx.hwnd, -1, LVNI_SELECTED);

    if (i == -1) return;

    LVITEM item = {0};
    item.mask = LVIF_PARAM;
    item.iItem = i;
    ListView_GetItem(g_ctx.hwnd, &item);
    AppActions::load_rom_from_path(g_ctx.discovered_roms[item.lParam].path);
}

LRESULT
notify(LPARAM lparam)
{
    switch (((LPNMHDR)lparam)->code)
    {
    case LVN_COLUMNCLICK: {
        auto lv = (LPNMLISTVIEW)lparam;

        if (g_config.rombrowser_sorted_column == lv->iSubItem)
        {
            g_config.rombrowser_sort_ascending ^= 1;
        }

        g_config.rombrowser_sorted_column = lv->iSubItem;

        rombrowser_update_sort();
        break;
    }
    case LVN_GETDISPINFO: {
        NMLVDISPINFO *plvdi = (NMLVDISPINFO *)lparam;
        const t_simple_rom_info &rombrowser_entry = g_ctx.discovered_roms[plvdi->item.lParam];
        switch (plvdi->item.iSubItem)
        {
        case 1: {
            // NOTE: The name may not be null-terminated, so we NEED to limit the
            // size
            char str[sizeof(core_rom_header::nom) + 1] = {0};
            if (strncpy_s(str, sizeof(str), (const char *)rombrowser_entry.header.nom, sizeof(core_rom_header::nom)) !=
                0)
            {
                g_view_logger->error("Failed to copy rom name");
            }
            StrNCpy(plvdi->item.pszText, IOUtils::to_wide_string(str).c_str(), plvdi->item.cchTextMax);
            break;
        }
        case 2: {
            wchar_t filename[MAX_PATH] = {0};
            _wsplitpath_s(rombrowser_entry.path.c_str(), nullptr, 0, nullptr, 0, filename, _countof(filename), nullptr,
                          0);
            StrNCpy(plvdi->item.pszText, filename, plvdi->item.cchTextMax);
            break;
        }
        case 3: {
            const auto size = std::to_wstring(rombrowser_entry.size / (1024 * 1024)) + L" MB";
            StrNCpy(plvdi->item.pszText, size.c_str(), plvdi->item.cchTextMax);
            break;
        }
        default:
            break;
        }

        break;
    }
    case LVN_KEYDOWN: {
        if (g_main_ctx.core_ctx->vr_get_core_executing())
        {
            break;
        }
        auto key = reinterpret_cast<LPNMLVKEYDOWN>(lparam)->wVKey;

        if (key == VK_RETURN)
        {
            invoke_selected_item();
            return TRUE;
        }

        break;
    }
    case NM_DBLCLK:
        invoke_selected_item();
        break;
    }
    return 0;
}

std::filesystem::path find_available_rom(const std::function<bool(const core_rom_header &)> &predicate)
{
    auto rom_paths = discover_roms();
    for (auto rom_path : rom_paths)
    {
        FILE *f = nullptr;
        if (_wfopen_s(&f, rom_path.c_str(), L"rb"))
        {
            g_view_logger->info(L"[Rombrowser] Failed to read file '{}'. Skipping!\n", rom_path.c_str());
            continue;
        }

        fseek(f, 0, SEEK_END);
        uint64_t len = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (len > sizeof(core_rom_header))
        {
            auto header = (core_rom_header *)malloc(sizeof(core_rom_header));
            fread(header, sizeof(core_rom_header), 1, f);

            g_main_ctx.core_ctx->vr_byteswap((uint8_t *)header);

            if (predicate(*header))
            {
                free(header);
                fclose(f);
                return rom_path;
            }

            free(header);
        }

        fclose(f);
    }

    return L"";
}

std::vector<std::filesystem::path> find_available_roms(const std::function<bool(const core_rom_header &)> &predicate)
{
    std::vector<std::filesystem::path> matching_roms;
    auto rom_paths = discover_roms();
    for (auto rom_path : rom_paths)
    {
        FILE *f = nullptr;
        if (_wfopen_s(&f, rom_path.c_str(), L"rb"))
        {
            g_view_logger->info(L"[Rombrowser] Failed to read file '{}'. Skipping!\n", rom_path.c_str());
            continue;
        }

        fseek(f, 0, SEEK_END);
        uint64_t len = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (len > sizeof(core_rom_header))
        {
            auto header = (core_rom_header *)malloc(sizeof(core_rom_header));
            fread(header, sizeof(core_rom_header), 1, f);

            g_main_ctx.core_ctx->vr_byteswap((uint8_t *)header);

            if (predicate(*header))
            {
                matching_roms.push_back(rom_path);
            }

            free(header);
        }

        fclose(f);
    }

    return matching_roms;
}

std::vector<t_simple_rom_info> get_discovered_roms()
{
    return g_ctx.discovered_roms;
}

void emu_launched_changed(std::any data)
{
    auto value = std::any_cast<bool>(data);
    ShowWindow(g_ctx.hwnd, !value ? SW_SHOW : SW_HIDE);
    EnableWindow(g_ctx.hwnd, !value);
    rombrowser_update_size();
}

void create()
{
    rombrowser_create();
    Messenger::subscribe(Messenger::Message::EmuLaunchedChanged, emu_launched_changed);
    Messenger::subscribe(Messenger::Message::StatusbarVisibilityChanged, [](auto _) { rombrowser_update_size(); });
    Messenger::subscribe(Messenger::Message::SizeChanged, [](auto _) { rombrowser_update_size(); });
    Messenger::subscribe(Messenger::Message::ConfigSaving, [](auto _) {
        for (int i = 0; i < g_config.rombrowser_column_widths.size(); ++i)
        {
            g_config.rombrowser_column_widths[i] = ListView_GetColumnWidth(g_ctx.hwnd, i);
        }
    });
}
} // namespace RomBrowser
