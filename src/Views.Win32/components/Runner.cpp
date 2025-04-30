/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <ThreadPool.h>
#include <Config.h>
#include <Main.h>
#include <Messenger.h>
#include <components/Runner.h>
#include <lua/LuaConsole.h>

static int32_t last_selected_id = -1;

static void run_auto(const int id, const std::filesystem::path& path)
{
    switch (id)
    {
    case IDC_LIST_ROMS:
        ThreadPool::submit_task([=] {
            const auto result = core_vr_start_rom(path);
            show_error_dialog_for_result(result);
        });
        break;
    case IDC_LIST_MOVIES:
        g_config.core.vcr_readonly = true;
        Messenger::broadcast(
        Messenger::Message::ReadonlyChanged,
        (bool)g_config.core.vcr_readonly);
        ThreadPool::submit_task([=] {
            core_vcr_start_playback(path);
        });
        break;
    case IDC_LIST_SCRIPTS:
        lua_create_and_run(path);
        break;
    default:
        break;
    }
}

static LRESULT CALLBACK dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        {
            last_selected_id = -1;
            auto populate_with_paths = [&](const int id, std::vector<std::wstring> paths) {
                auto ctl = GetDlgItem(hwnd, id);
                for (auto path : paths)
                {
                    auto index = ListBox_AddString(ctl, std::filesystem::path(path).filename().wstring().c_str());

                    auto buffer = new wchar_t[path.size() + 1]();
                    memcpy(buffer, path.data(), path.size() * sizeof(wchar_t));

                    ListBox_SetItemData(ctl, index, reinterpret_cast<LPARAM>(buffer));
                }
            };
            populate_with_paths(IDC_LIST_ROMS, g_config.recent_rom_paths);
            populate_with_paths(IDC_LIST_MOVIES, g_config.recent_movie_paths);
            populate_with_paths(IDC_LIST_SCRIPTS, g_config.recent_lua_script_paths);
            break;
        }
    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        break;
    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDC_LIST_ROMS:
        case IDC_LIST_MOVIES:
        case IDC_LIST_SCRIPTS:
            if (HIWORD(wparam) == LBN_DBLCLK)
            {
                auto index = ListBox_GetCurSel(GetDlgItem(hwnd, LOWORD(wparam)));

                if (index == -1)
                {
                    break;
                }

                auto buffer = (char*)ListBox_GetItemData(GetDlgItem(hwnd, LOWORD(wparam)), index);
                std::filesystem::path path(buffer);
                delete buffer;

                EndDialog(hwnd, IDOK);

                run_auto(LOWORD(wparam), path);
            }
            if (HIWORD(wparam) == LBN_SELCHANGE)
            {
                // Clear the selections of the other items
                for (auto id : {IDC_LIST_ROMS, IDC_LIST_MOVIES, IDC_LIST_SCRIPTS})
                {
                    if (id == LOWORD(wparam))
                    {
                        continue;
                    }
                    ListBox_SetCurSel(GetDlgItem(hwnd, id), -1);
                }

                last_selected_id = LOWORD(wparam);
            }
            break;
        case IDOK:
            {
                if (!last_selected_id)
                {
                    break;
                }

                SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(last_selected_id, LBN_DBLCLK), 0);
                break;
            }
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
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

void Runner::show()
{
    DialogBox(g_app_instance, MAKEINTRESOURCE(IDD_RUNNER), g_main_hwnd, (DLGPROC)dlgproc);
}
