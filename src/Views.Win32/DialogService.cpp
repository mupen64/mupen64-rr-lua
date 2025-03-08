/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Config.h>
#include <DialogService.h>
#include <gui/Loggers.h>
#include <gui/Main.h>
#include <gui/features/Statusbar.h>
#include <lua/LuaConsole.h>

size_t DialogService::show_multiple_choice_dialog(const std::vector<std::wstring>& choices, const wchar_t* str, const wchar_t* title, core_dialog_type type, void* hwnd)
{
    std::vector<TASKDIALOG_BUTTON> buttons;

    buttons.reserve(choices.size());
    for (int i = 0; i < choices.size(); ++i)
    {
        buttons.push_back({i, choices[i].c_str()});
    }

    auto icon = TD_ERROR_ICON;
    switch (type)
    {
    case fsvc_error:
        icon = TD_ERROR_ICON;
        break;
    case fsvc_warning:
        icon = TD_WARNING_ICON;
        break;
    case fsvc_information:
        icon = TD_INFORMATION_ICON;
        break;
    }

    TASKDIALOGCONFIG task_dialog_config = {
    .cbSize = sizeof(TASKDIALOGCONFIG),
    .hwndParent = static_cast<HWND>(hwnd ? hwnd : g_main_hwnd),
    .pszWindowTitle = title,
    .pszMainIcon = icon,
    .pszContent = str,
    .cButtons = (UINT)buttons.size(),
    .pButtons = buttons.data(),
    };

    int pressed_button = -1;
    TaskDialogIndirect(&task_dialog_config, &pressed_button, NULL, NULL);

    g_view_logger->trace(L"[FrontendService] show_multiple_choice_dialog: '{}', answer: {}", str, pressed_button > 0 ? choices[pressed_button] : L"?");

    return pressed_button;
}

bool DialogService::show_ask_dialog(const wchar_t* str, const wchar_t* title, bool warning, void* hwnd)
{
    g_view_logger->trace(L"[FrontendService] show_ask_dialog: '{}'", str);
    if (g_config.silent_mode)
    {
        return true;
    }
    return MessageBox(static_cast<HWND>(hwnd ? hwnd : g_main_hwnd), str, title, MB_YESNO | (warning ? MB_ICONWARNING : MB_ICONQUESTION)) == IDYES;
}

void DialogService::show_dialog(const wchar_t* str, const wchar_t* title, core_dialog_type type, void* hwnd)
{
    int icon = 0;

    switch (type)
    {
    case fsvc_error:
        g_view_logger->error(L"[FrontendService] {}", str);
        icon = MB_ICONERROR;
        break;
    case fsvc_warning:
        g_view_logger->warn(L"[FrontendService] {}", str);
        icon = MB_ICONWARNING;
        break;
    case fsvc_information:
        g_view_logger->info(L"[FrontendService] {}", str);
        icon = MB_ICONINFORMATION;
        break;
    default:
        assert(false);
    }

    if (!g_config.silent_mode)
    {
        MessageBox(static_cast<HWND>(hwnd ? hwnd : g_main_hwnd), str, title, icon);
    }
}

void DialogService::show_statusbar(const wchar_t* str)
{
    Statusbar::post(str);
}
