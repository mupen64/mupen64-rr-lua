/*
 * Copyright (c) 2025, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

extern HINSTANCE g_inst;
extern core_plugin_extended_funcs* g_ef;

#define PLUGIN_VERSION L"2.0.0-rc4"

#ifdef _M_X64
#define PLUGIN_ARCH L" x64"
#else
#define PLUGIN_ARCH L" "
#endif

#ifdef _DEBUG
#define PLUGIN_TARGET L" Debug"
#else
#define PLUGIN_TARGET L" "
#endif

#define PLUGIN_NAME L"TASInput " PLUGIN_VERSION PLUGIN_ARCH PLUGIN_TARGET

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
