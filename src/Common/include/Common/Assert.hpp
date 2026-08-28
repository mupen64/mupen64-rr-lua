/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief Asserts a condition at runtime.
 */
#define NEED(condition, message)                                                                                       \
    if (!(condition)) throw std::logic_error(message)

#ifdef _WIN32

/**
 * \brief Asserts that an HRESULT is SUCCESS at runtime.
 */
#define NEED_HR(hr, message) NEED(!FAILED(hr), message)

#endif
