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
    HWND subheader_hwnd{};
    HWND secondary_hwnd{};
    HWND combo_hwnd{};
    HWND edit_hwnd{};
    HWND status_hwnd{};

    ActionManager::action_path action_path{};
    size_t param_index{};
    std::vector<ActionManager::t_action_param> ref_params{};
    ActionManager::action_argument_map filled_params{};

    std::vector<std::function<void()>> unsubscribe_funcs{};

    DLGTEMPLATEEX *dlg_template;
};

enum class NextParamResult
{
    Failed = 0,
    Continue = 1,
    Finished = 2
};

static t_parameter_palette_context g_ctx{};

/**
 * \brief Updates the UI and resets the editbox when the current parameter page changes.
 */
static void on_page_changed()
{
    const auto &current_param = g_ctx.ref_params[g_ctx.param_index];

    SetWindowText(g_ctx.subheader_hwnd, std::format(L"{}:", current_param.name).c_str());

    const auto param_number = g_ctx.param_index + 1;
    const auto total_params = g_ctx.ref_params.size();
    SetWindowText(g_ctx.secondary_hwnd, std::format(L"Step {}/{}", param_number, total_params).c_str());

    const std::wstring initial_value = current_param.get_initial_value ? current_param.get_initial_value() : L"";
    SetWindowText(g_ctx.edit_hwnd, initial_value.c_str());

    SetWindowText(g_ctx.status_hwnd, L"");
}

/**
 * \brief Tries to apply and store the current parameter input.
 */
static bool try_apply_parameter()
{
    const auto &current_param = g_ctx.ref_params[g_ctx.param_index];

    const auto input = get_window_text(g_ctx.edit_hwnd).value();

    // 1. Update hints
    ComboBox_ResetContentKeepEdit(g_ctx.combo_hwnd);
    if (current_param.get_hints)
    {
        const auto hints = current_param.get_hints(input);
        for (const auto &hint : hints)
        {
            ComboBox_AddString(g_ctx.combo_hwnd, hint.c_str());
        }
    }

    // 2. Validate input
    const auto &validator = current_param.validator;
    const auto validation_result = validator(input);
    if (validation_result.has_value())
    {
        const auto validation_message = validation_result.value();
        SetWindowText(g_ctx.status_hwnd, std::format(L"⚠️ {}", validation_message).c_str());
        return false;
    }

    g_ctx.filled_params[current_param.key] = input;

    SetWindowText(g_ctx.status_hwnd, L"✔️ Press Enter to confirm.");

    return true;
}

/**
 * \brief Advances to the next parameter in parameter input mode.
 */
static NextParamResult next_parameter()
{
    if (!try_apply_parameter())
    {
        return NextParamResult::Failed;
    }

    g_ctx.param_index++;

    if (g_ctx.param_index >= g_ctx.ref_params.size())
    {
        SendMessage(g_ctx.hwnd, WM_CLOSE, 0, 0);
        ActionManager::invoke(g_ctx.action_path, false, true, g_ctx.filled_params);
        return NextParamResult::Finished;
    }

    on_page_changed();

    return NextParamResult::Continue;
}

/**
 * \brief Tries to skip to the end by applying the current parameter and then all future ones with their initial values.
 */
static void try_skip_to_end()
{
    while (true)
    {
        const auto result = next_parameter();
        if (result == NextParamResult::Failed || result == NextParamResult::Finished)
        {
            break;
        }
    }
}

/**
 * \brief Updates the dialog position and size based on the parent window.
 */
static void update_dialog_position_and_size()
{
    RECT parent_rc{};
    GetClientRect(g_main_ctx.hwnd, &parent_rc);

    constexpr auto margin = 10;
    const auto width = std::max(400L, parent_rc.right / 3 - margin);

    RECT rc;
    rc.left = parent_rc.right / 2 - width / 2;
    rc.top = margin;
    rc.right = rc.left + width;
    rc.bottom = rc.top + g_ctx.dlg_template->cy;

    MapWindowRect(g_main_ctx.hwnd, HWND_DESKTOP, &rc);
    SetWindowPos(g_ctx.hwnd, nullptr, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                 SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
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
            if ((GetKeyState(VK_CONTROL) & 0x8000) != 0)
                try_skip_to_end();
            else
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
        g_ctx.subheader_hwnd = GetDlgItem(hwnd, IDC_PARAMETER_PALETTE_SUBHEADER);
        g_ctx.secondary_hwnd = GetDlgItem(hwnd, IDC_PARAMETER_PALETTE_SECONDARY);
        g_ctx.combo_hwnd = GetDlgItem(hwnd, IDC_PARAMETER_PALETTE_COMBO);
        g_ctx.status_hwnd = GetDlgItem(hwnd, IDC_PARAMETER_PALETTE_STATUS);

        COMBOBOXINFO combo_info{};
        combo_info.cbSize = sizeof(COMBOBOXINFO);
        GetComboBoxInfo(g_ctx.combo_hwnd, &combo_info);
        g_ctx.edit_hwnd = combo_info.hwndItem;

        // 1. Remove the titlebar and prevent resizing.
        const LONG style = GetWindowLong(hwnd, GWL_STYLE);
        SetWindowLong(hwnd, GWL_STYLE, style & ~WS_CAPTION);
        attach_no_resize_subproc(hwnd);

        // 2. Add resize anchors
        ResizeAnchor::add_anchors(hwnd, {
                                            {g_ctx.header_hwnd, ResizeAnchor::HORIZONTAL_ANCHOR},
                                            {g_ctx.subheader_hwnd, ResizeAnchor::HORIZONTAL_ANCHOR},
                                            {g_ctx.secondary_hwnd, ResizeAnchor::HORIZONTAL_ANCHOR},
                                            {g_ctx.combo_hwnd, ResizeAnchor::HORIZONTAL_ANCHOR},
                                            {g_ctx.status_hwnd, ResizeAnchor::HORIZONTAL_ANCHOR},
                                        });

        // 3. Set the focus to the edit control
        SetFocus(g_ctx.combo_hwnd);

        // 4. Subclass the controls for key event handling
        SetWindowSubclass(g_ctx.edit_hwnd, keyboard_interaction_subclass_proc, 0, 0);

        // 5. Initialize header
        const auto display_name = ActionManager::get_display_name(g_ctx.action_path);
        SetWindowText(g_ctx.header_hwnd, std::format(L"{}", display_name).c_str());

        on_page_changed();
        update_dialog_position_and_size();
        try_apply_parameter();

        g_ctx.unsubscribe_funcs.push_back(Messenger::subscribe(
            Messenger::Message::MainWindowMoved, [](const auto &) { update_dialog_position_and_size(); }));

        g_ctx.unsubscribe_funcs.push_back(Messenger::subscribe(
            Messenger::Message::SizeChanged, [](const auto &) { update_dialog_position_and_size(); }));
        break;
    }
    case WM_DESTROY:
        for (const auto &unsubscribe_func : g_ctx.unsubscribe_funcs)
        {
            unsubscribe_func();
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDC_PARAMETER_PALETTE_COMBO:
            switch (HIWORD(wparam))
            {
            case CBN_EDITCHANGE:
                try_apply_parameter();
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
        break;
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

    if (!g_ctx.dlg_template)
    {
        const auto result = load_resource_as_dialog_template(IDD_PARAMETER_PALETTE, &g_ctx.dlg_template);
        RT_ASSERT(result, L"Failed to load parameter palette dialog template");
    }

    const HWND hwnd = CreateDialog(g_main_ctx.hinst, MAKEINTRESOURCE(IDD_PARAMETER_PALETTE), g_main_ctx.hwnd, dlgproc);
    ShowWindow(hwnd, SW_SHOW);
}

HWND ParameterPalette::hwnd()
{
    return g_ctx.hwnd;
}
