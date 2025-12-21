/*
 * Copyright (c) 2025, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

static void set_style(HWND hwnd, int domain, int style, bool value)
{
    auto base = GetWindowLongA(hwnd, domain);

    if (value)
    {
        SetWindowLongA(hwnd, domain, base | style);
    }
    else
    {
        SetWindowLongA(hwnd, domain, base & ~style);
    }
}

static bool is_mouse_over_control(const HWND control_hwnd)
{
    POINT pt;
    RECT rect;

    GetCursorPos(&pt);
    if (GetWindowRect(control_hwnd, &rect)) // failed to get the dimensions
        return (pt.x <= rect.right && pt.x >= rect.left && pt.y <= rect.bottom && pt.y >= rect.top);
    return FALSE;
}

static bool is_mouse_over_control(const HWND hwnd, const int id)
{
    return is_mouse_over_control(GetDlgItem(hwnd, id));
}
