/*
 * Copyright (c) 2025, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

static void set_style(const HWND hwnd, const int domain, const long style, const bool value)
{
    auto base = GetWindowLong(hwnd, domain);

    if (value)
    {
        SetWindowLong(hwnd, domain, base | style);
    }
    else
    {
        SetWindowLong(hwnd, domain, base & ~style);
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

static void runtime_assert_fail(const std::wstring &message)
{
#if defined(_DEBUG)
    __debugbreak();
#endif
    MessageBox(nullptr, message.c_str(), L"Failed Runtime Assertion", MB_ICONERROR | MB_OK);
    std::terminate();
}

/**
 * \brief Asserts a condition at runtime.
 */
#define RT_ASSERT(condition, message)                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            runtime_assert_fail(message);                                                                              \
        }                                                                                                              \
    } while (0)

/**
 * \brief Asserts that an HRESULT is SUCCESS at runtime.
 */
#define RT_ASSERT_HR(hr, message) RT_ASSERT(!FAILED(hr), message)
