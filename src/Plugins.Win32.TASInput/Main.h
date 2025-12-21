/*
 * Copyright (c) 2025, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

extern HINSTANCE g_inst;
extern core_plugin_extended_funcs* g_ef;

#define PLUGIN_NAME VERSION_NAME_HELPER_GEN_NAME(L"TAS Input", L"2.0.0")

#define NUMBER_OF_CONTROLS 4

static void runtime_assert_fail(const std::wstring& message)
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
#define RT_ASSERT(condition, message)     \
    do                                    \
    {                                     \
        if (!(condition))                 \
        {                                 \
            runtime_assert_fail(message); \
        }                                 \
    }                                     \
    while (0)

/**
 * \brief Asserts that an HRESULT is SUCCESS at runtime.
 */
#define RT_ASSERT_HR(hr, message) RT_ASSERT(!FAILED(hr), message)
