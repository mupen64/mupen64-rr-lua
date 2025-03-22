/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once


/// <summary>
/// A helper for generating exception logs
/// </summary>
namespace CrashHelper
{
    /**
     * Logs a crash to the view logger.
     * \param exception_pointers_ptr The exception pointers
     */
    void log_crash(_EXCEPTION_POINTERS* exception_pointers_ptr);
} // namespace CrashHelper
