/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <DialogService.h>
#include <components/Cheats.h>

static LRESULT CALLBACK dlgproc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
    case WM_INITDIALOG:
        {
            std::stack<std::vector<core_cheat>> override_stack;
            core_cht_get_override_stack(override_stack);
            if (!override_stack.empty())
            {
                SetDlgItemText(hwnd, IDC_CHEAT_STATUS, L"Read-only: Cheats are overriden by the core.");
            }
            goto rebuild_list;
        }
    case WM_DESTROY:

        break;
    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_LIST_CHEATS:
            goto update_selection;
        case IDC_NEW_CHEAT:
            {
                core_cheat script;

                if (!core_cht_compile(L"D033AFA1 0020\n8133B1BC 4220\nD033AFA1 0020\n8133B17C 0300\nD033AFA1 0020\n8133B17E 0880", script))
                {
                    break;
                }

                std::vector<core_cheat> cheats;
                core_cht_get_list(cheats);
                cheats.push_back(script);
                core_cht_set_list(cheats);

                goto rebuild_list;
            }
        case IDC_REMOVE_CHEAT:
            {
                auto lb_hwnd = GetDlgItem(hwnd, IDC_LIST_CHEATS);
                auto selected_index = ListBox_GetCurSel(lb_hwnd);
                if (selected_index == -1)
                {
                    break;
                }

                std::vector<core_cheat> cheats;
                core_cht_get_list(cheats);
                cheats.erase(cheats.begin() + selected_index);
                core_cht_set_list(cheats);

                goto rebuild_list;
            }
        case IDC_CHECK_CHEAT_ENABLED:
            {
                auto lb_hwnd = GetDlgItem(hwnd, IDC_LIST_CHEATS);
                auto selected_index = ListBox_GetCurSel(lb_hwnd);
                if (selected_index == -1)
                {
                    break;
                }

                std::vector<core_cheat> cheats;
                core_cht_get_list(cheats);
                cheats[selected_index].active = IsDlgButtonChecked(hwnd, IDC_CHECK_CHEAT_ENABLED) == BST_CHECKED;
                core_cht_set_list(cheats);

                goto rebuild_list;
            }

        case IDC_CHEAT_APPLY:
            {
                auto lb_hwnd = GetDlgItem(hwnd, IDC_LIST_CHEATS);
                auto selected_index = ListBox_GetCurSel(lb_hwnd);
                if (selected_index == -1)
                {
                    break;
                }

                std::vector<core_cheat> cheats;
                core_cht_get_list(cheats);
                const bool prev_active = cheats[selected_index].active;

                wchar_t code[4096] = {};
                Edit_GetText(GetDlgItem(hwnd, IDC_EDIT_CHEAT), code, sizeof(code));

                wchar_t name[256] = {};
                Edit_GetText(GetDlgItem(hwnd, IDC_EDIT_CHEAT_NAME), name, sizeof(name));

                core_cheat script;

                if (!core_cht_compile(code, script))
                {
                    DialogService::show_dialog(L"Cheat code could not be compiled.\r\nVerify that the syntax is correct", L"Cheats", fsvc_error);
                    break;
                }

                script.name = name;
                script.active = prev_active;

                cheats[selected_index] = script;
                core_cht_set_list(cheats);

                goto rebuild_list;
            }
        default:
            break;
        }
        break;

    default:
        break;
    }
    return FALSE;

update_selection:
    {
        auto selected_index = ListBox_GetCurSel(GetDlgItem(hwnd, IDC_LIST_CHEATS));
        if (selected_index == -1)
        {
            return FALSE;
        }

        std::vector<core_cheat> cheats;
        core_cht_get_list(cheats);

        CheckDlgButton(hwnd, IDC_CHECK_CHEAT_ENABLED, cheats[selected_index].active ? BST_CHECKED : BST_UNCHECKED);
        SetDlgItemText(hwnd, IDC_EDIT_CHEAT, cheats[selected_index].code.c_str());
        Edit_SetText(GetDlgItem(hwnd, IDC_EDIT_CHEAT_NAME), cheats[selected_index].name.c_str());
    }
    return FALSE;


rebuild_list:
    {
        auto lb_hwnd = GetDlgItem(hwnd, IDC_LIST_CHEATS);
        auto prev_index = ListBox_GetCurSel(lb_hwnd);
        ListBox_ResetContent(lb_hwnd);
        std::vector<core_cheat> cheats;
        core_cht_get_list(cheats);
        for (const auto& script : cheats)
        {
            auto name = !script.active
            ? script.name + L" (Disabled)"
            : script.name;
            ListBox_AddString(lb_hwnd, name.c_str());
        }
        ListBox_SetCurSel(lb_hwnd, prev_index);
        goto update_selection;
    }
}

void Cheats::show()
{
    DialogBox(g_app_instance,
              MAKEINTRESOURCE(IDD_CHEATS),
              g_main_hwnd,
              (DLGPROC)dlgproc);
}
