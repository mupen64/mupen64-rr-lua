/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Common.Views/App.hpp>

/**
 * \brief Asserts a condition at runtime.
 */
#define RT_ASSERT(condition, message)                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition)) app_runtime_assert_fail(message);                                                            \
    } while (0)

#ifdef _WIN32

/**
 * \brief Asserts that an HRESULT is SUCCESS at runtime.
 */
#define RT_ASSERT_HR(hr, message) RT_ASSERT(!FAILED(hr), message)

#endif
