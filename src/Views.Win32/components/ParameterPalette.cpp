/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "RomBrowser.h"
#include "stdafx.h"
#include <components/CommandPalette.h>
#include <components/ConfigDialog.h>
#include <components/AppActions.h>
#include <Messenger.h>
#include "ParameterPalette.h"

struct t_parameter_palette_context
{
    HWND hwnd{};
    HWND header_hwnd{};
    HWND edit_hwnd{};
    HWND status_hwnd{};

    ActionManager::action_path action_path{};
    size_t param_index{};
    std::vector<ActionManager::t_action_param> ref_params{};
    ActionManager::action_parameter_list filled_params{};
};

static t_parameter_palette_context g_ctx{};

static void update_header()
{
    const auto &current_param = g_ctx.ref_params[g_ctx.param_index];
    SetWindowText(g_ctx.header_hwnd, std::format(L"Enter value for '{}':", current_param.name).c_str());
}
/**
 * \brief Advances to the next parameter in parameter input mode.
 */
static void next_parameter()
{
    auto &current_param = g_ctx.ref_params[g_ctx.param_index];

    const auto input = get_window_text(g_ctx.edit_hwnd).value();

    const auto &validator = current_param.validator;
    const auto validation_result = validator(input);
    if (validation_result.has_value())
    {
        const auto validation_message = validation_result.value();
        SetWindowText(g_ctx.status_hwnd, std::format(L"Validation failed: '{}'", validation_message).c_str());
        ShowWindow(g_ctx.status_hwnd, SW_SHOW);
        return;
    }
    g_ctx.filled_params[current_param.key] = input;

    g_ctx.param_index++;

    if (g_ctx.param_index >= g_ctx.ref_params.size())
    {
        // All parameters filled, invoke the action.
        SendMessage(g_ctx.hwnd, WM_CLOSE, 0, 0);
        ActionManager::invoke(g_ctx.action_path, false, true, g_ctx.filled_params);
        return;
    }

    update_header();
    SetWindowText(g_ctx.edit_hwnd, L"");

    ShowWindow(g_ctx.status_hwnd, SW_HIDE);
}

static LRESULT CALLBACK keyboard_interaction_subclass_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                                           UINT_PTR id, DWORD_PTR ref_data)
{
    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, keyboard_interaction_subclass_proc, id);
        break;
    case WM_GETDLGCODE:
        return DLGC_WANTALLKEYS;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wparam == VK_ESCAPE)
        {
            SendMessage(g_ctx.hwnd, WM_CLOSE, 0, 0);
            return FALSE;
        }
        if (wparam == VK_RETURN)
        {
            next_parameter();
            return FALSE;
        }
        break;
    default:
        break;
    }
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

static INT_PTR CALLBACK dlgproc(const HWND hwnd, const UINT msg, const WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG: {
        g_ctx.hwnd = hwnd;
        g_ctx.header_hwnd = GetDlgItem(hwnd, IDC_PARAMETER_PALETTE_HEADER);
        g_ctx.edit_hwnd = GetDlgItem(hwnd, IDC_PARAMETER_PALETTE_EDIT);
        g_ctx.status_hwnd = GetDlgItem(hwnd, IDC_PARAMETER_PALETTE_STATUS);

        // 1. Remove the titlebar
        const LONG style = GetWindowLong(hwnd, GWL_STYLE);
        SetWindowLong(hwnd, GWL_STYLE, style & ~WS_CAPTION);

        // 2. Add resize anchors
        ResizeAnchor::add_anchors(hwnd, {
                                            {g_ctx.header_hwnd, ResizeAnchor::HORIZONTAL_ANCHOR},
                                            {g_ctx.edit_hwnd, ResizeAnchor::HORIZONTAL_ANCHOR},
                                            {g_ctx.status_hwnd, ResizeAnchor::HORIZONTAL_ANCHOR},
                                        });

        // 3. Set a reasonable position and size for the dialog (centered horizontally, vertically top-justified)
        RECT parent_rc{};
        GetClientRect(g_main_ctx.hwnd, &parent_rc);

        constexpr auto margin = 10;
        const auto width = std::max(400L, parent_rc.right / 3 - margin);

        RECT rc;
        rc.left = parent_rc.right / 2 - width / 2;
        rc.top = margin;
        rc.right = rc.left + width;
        rc.bottom = rc.top + 90L;

        MapWindowRect(g_main_ctx.hwnd, HWND_DESKTOP, &rc);
        SetWindowPos(hwnd, nullptr, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                     SWP_NOZORDER | SWP_FRAMECHANGED);

        // 4. Set the focus to the edit control
        SetFocus(g_ctx.edit_hwnd);

        // 5. Subclass the controls for key event handling
        SetWindowSubclass(g_ctx.edit_hwnd, keyboard_interaction_subclass_proc, 0, 0);
        
        update_header();

        break;
    }
    case WM_CLOSE:
        DestroyWindow(g_ctx.hwnd);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

void ParameterPalette::show(const ActionManager::action_path &action_path)
{
    g_ctx = {};
    g_ctx.action_path = action_path;
    g_ctx.ref_params = ActionManager::get_params(action_path);

    const HWND hwnd = CreateDialog(g_main_ctx.hinst, MAKEINTRESOURCE(IDD_PARAMETER_PALETTE), g_main_ctx.hwnd, dlgproc);
    ShowWindow(hwnd, SW_SHOW);
}

HWND ParameterPalette::hwnd()
{
    return g_ctx.hwnd;
}
