/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief Asserts a condition at runtime.
 * \param condition The condition to assert.
 * \param message The message to display if the condition is not met.
 */
void need(bool condition, std::string_view message = "");

#ifdef _WIN32
/**
 * \brief Asserts an integer condition at runtime.
 *
 * This overload avoids ambiguity with HRESULT on Windows, where HRESULT is a
 * typedef of a signed integer type.
 *
 * \param condition The integer condition to assert.
 * \param message The message to display if the condition is not met.
 */
void need(int condition, std::string_view message = "");
/**
 * \brief Asserts a HRESULT condition at runtime.
 * \param hr The HRESULT to assert.
 * \param message The message to display if the condition is not met.
 */
void need(HRESULT hr, std::string_view message);
#endif
