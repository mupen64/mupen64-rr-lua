/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <components/TextEditDialog.h>

struct t_text_edit_dialog_context
{
    TextEditDialog::t_params params{};
};

static t_text_edit_dialog_context g_ctx{};

static INT_PTR CALLBACK about_dlg_proc(const HWND hwnd, const UINT message, const WPARAM w_param, LPARAM)
{
    switch (message)
    {
    case WM_INITDIALOG:
        SetWindowText(hwnd, g_ctx.params.caption.c_str());
        SetWindowText(GetDlgItem(hwnd, IDC_EDIT), g_ctx.params.text.c_str());
        Edit_SetReadOnly(GetDlgItem(hwnd, IDC_EDIT), g_ctx.params.readonly ? TRUE : FALSE);
        SetFocus(GetDlgItem(hwnd, IDC_EDIT));
        return FALSE;
    case WM_DESTROY:
        g_ctx.params.text = get_window_text(GetDlgItem(hwnd, IDC_EDIT)).value_or(L"");
        break;
    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        break;
    case WM_COMMAND:
        switch (LOWORD(w_param))
        {
        case IDOK:
            EndDialog(hwnd, IDOK);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
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

std::optional<std::wstring> TextEditDialog::show(const t_params &params)
{
    g_ctx = {};
    g_ctx.params = params;

    const auto result = DialogBox(g_main_ctx.hinst, MAKEINTRESOURCE(IDD_TEXT_EDIT), params.parent_hwnd, about_dlg_proc);

    if (result == IDCANCEL)
    {
        return std::nullopt;
    }

    return g_ctx.params.text;
}
